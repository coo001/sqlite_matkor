#include <cstring>
#include <fstream>
#include <vector>
#include <regex>
#include <iostream>

//Database file
/*

header info
�տ� �ִ� ���� ���� magic string
4���� => Database page size

*/
/*
5�� 15��
SQLite�� dbinfo���� �ٸ� ��ɾ �ִ�
.tables
���̺� �̸� ���
SQL Database Schema�� �������
tbl_name txt�� �ִ�
Gtableoranges���� �����ؾ���

1. sqlite schema page�� shell pointer �迭 ã��
2. �ױ� offset�� �־ table name ���٤���
3. schema�� table b tree�� �������
leaf shell�� 3���� type���� ����
���ڵ� ũ��(varint), rowid(varint), record(record)
record =>header, body
header => header size(varint), record each row ���� type code(varint)
body => record row value(�������)

0ec3 -> 0f3c���� ������
ù ����Ʈ�� record size
������ ����Ʈ�� record header size
4 -> schema type
5 -> schema name
6 -> table name size
7 -> root page type
8 9 -> sql serial type

17�� ����
23 - 13 / 2 => 5
�� 13��?
(¦�� �϶� 12
 sql

 6��° byte�� ���� �� �� Ȯ���ؼ� -13 /2 �ϸ�
 ��ġ�� ����

 ���� ���� :���̺� �ִ� �� �ڵ��� ������ ����ϱ�
 sqlite.exe sample.db "SELECT COUNT(*) FROM apples' - apples�� ������, �Ӱ� command
���� �� �ɷ� ������

���� �ð��� : �� �ڿ� �ڵ尡 ��� �ٲ�� �� �����Ұ���
*/

/*
5.29
SELECT NAME from apples
record format�� ���ؼ� �� �����Ѵ�
sqlite_schema�� sql�� ���ؼ� ������ �� �ִ�

1. create statement �����;���
2. �ױ⼭ data�� ���� �;���

���� ����
SELECT name, color from apples
SELECT name, color FROM apples WHERE color = 'Yellow'

*/

unsigned short get_num_tables(std::ifstream& database_file){
    char buffer2[2];

    database_file.seekg(103);
    database_file.read(buffer2,2);
    unsigned short table_num = (static_cast<unsigned char>(buffer2[1])|
                                    (static_cast<unsigned char>(buffer2[0])<<8));

    return table_num;
}

uint64_t parse_varint(std::ifstream& database_file){
    uint64_t ret = 0;
    char byte = database_file.get();
    int idx = 0;

    while(byte & 0b10000000 && idx < 8){
        ret = (ret<<7)+(byte&0b01111111);
        byte = database_file.get();
        idx++;
    }

    if(idx <= 8){
        ret = (ret<<7)+(byte&0b01111111);
    }
    else{
        ret = (ret<<8)+byte;
    }
    return ret;
}

unsigned short get_page_size(std::ifstream& database_file)
{
    database_file.seekg(16);

    char buffer[2];
    database_file.read(buffer, 2);

    unsigned short page_size = (static_cast<unsigned char>(buffer[1]) |
                                (static_cast<unsigned char>(buffer[0]) << 8));
    return page_size;
}

uint64_t bytes_to_int(const std::string& s){
    uint64_t ret = 0;

    for(int i=0;i<s.size();++i){
        ret |= static_cast<uint64_t>(static_cast<uint8_t>(s[i])<<(8*(s.size()-i-1)));
    }
    return ret;
}

//data���� �� leaf page�� ����Ǿ� ����
//�׷��⿡ �츮�� leaf page�� �߿��ϰ� ������


int query_leaf_page(std::ifstream& database_file, uint64_t base, std::vector<std::string>& columns, std::vector<int>& column_indices, int where_column_idx, int where_idx, std::string where_arg)
{
    char buffer[2];

    database_file.seekg(base+3);
    database_file.read(buffer,2);

    uint64_t count = (static_cast<unsigned char>(buffer[1]) |
                    (static_cast<unsigned char>(buffer[0]) << 8));

    for (int i = 0; i < count; ++i)
    {
        database_file.seekg(base + 8 + 2 * i);
        database_file.read(buffer, 2);

        unsigned short cell_offset =
            (static_cast<unsigned char>(buffer[1]) |
            (static_cast<unsigned char>(buffer[0]) << 8));

        database_file.seekg(base + cell_offset);

        uint64_t record_size = parse_varint(database_file);
        uint64_t row_id = parse_varint(database_file);
        uint64_t record_header_size = parse_varint(database_file);

        std::vector<uint64_t> headers;

        for (int j = 0; j < columns.size(); ++j)
        {
            headers.push_back(parse_varint(database_file));
        }

        std::vector<std::string> data;

        for (int j = 0; j < columns.size(); ++j)
        {
            std::string temp;
            if (headers[j] == 0)
            {
                // Do nothing
            }
            else if (headers[j] <= 7)
            {
                uint64_t size = headers[j];

                if (headers[j] == 5)
                {
                    size = 6;
                }
                else if (headers[j] == 6)
                {
                    size = 8;
                }

                temp.resize(size, '_');
                database_file.read(&temp[0], size);
            }
            else if(headers[j]==8){
                temp.push_back(0);
            }
            else if(headers[j]==9){
                temp.push_back(1);
            }
            else if (headers[j] >= 12 && headers[j] % 2 == 0)
            {
                uint64_t size = (headers[j] - 12) / 2;
                temp.resize(size, '_');
                database_file.read(&temp[0], size);
            }
            else if (headers[j] >= 13 && headers[j] % 2 == 1)
            {
                uint64_t size = (headers[j] - 13) / 2;
                temp.resize(size, '_');
                database_file.read(&temp[0], size);
            }
            else
            {
                std::cerr << "Error parsing data" << headers[j] << std::endl;
                return 1;
            }

            data.push_back(temp);
        }

        if (where_idx != -1 && data[where_column_idx] != where_arg)
        {
            continue;
        }
        for (int j = 0; j < column_indices.size(); ++j)
        {
            if(column_indices[j]==0){
                std::cout << row_id;
            }
            else if(headers[column_indices[j]]<=9){
                std::cout << bytes_to_int(data[column_indices[j]]);
            }
            else{
                std::cout << data[column_indices[j]];
            }

            if (j != column_indices.size() - 1)
            {
                std::cout << "|";
            }
        }

        std::cout << std::endl;
    }

    return 0;
}

std::vector<std::string> split(std::string s, char c)
{
    std::vector<std::string> res;
    std::string temp;

    for (char x : s)
    {
        if (x == c)
        {
            res.push_back(temp);
            temp = "";
        }
        else
        {
            temp.push_back(x);
        }
    }

    res.push_back(temp);
    return res;
}

int traverse_btree(std::ifstream& database_file, uint64_t base, std::vector<std::string>& columns,std::vector<int>& column_indices, int where_column_idx, int where_idx, std::string where_arg)
{
    database_file.seekg(base);

    uint8_t type = database_file.get();

    //leaf page
    if(type==10||type==13)
    {
        if(query_leaf_page(database_file,base,columns,column_indices,where_column_idx,where_idx,where_arg)!=0)
        {
            return -1;
        }
        return 0;
    }

    //interior page
    char buffer[4];

    database_file.seekg(base+3);
    database_file.read(buffer,2);

    uint64_t count = (static_cast<unsigned char>(buffer[1])|
                        (static_cast<unsigned char>(buffer[0])<<8));

    unsigned short page_size = get_page_size(database_file);

    for(int i=0;i<count;i++)
    {
        database_file.seekg(base+12+2*i);
        database_file.read(buffer,2);

        unsigned short cell_offset =
            (static_cast<unsigned char>(buffer[1])|
             (static_cast<unsigned char>(buffer[0])<<8));

        database_file.seekg(base+cell_offset);
        database_file.read(buffer,4);

        uint64_t page_num = (static_cast<unsigned char>(buffer[3])|
                             (static_cast<unsigned char>(buffer[2])<<8)|
                             (static_cast<unsigned char>(buffer[2])<<16)|
                             (static_cast<unsigned char>(buffer[2])<<24));

        if(traverse_btree(database_file,(page_num-1)*page_size,columns,column_indices,where_column_idx,where_idx,where_arg)!=0)
        {
            return -1;
        }
    }
    return 0;
}

 //SqLite Varint
 /*
 ����Ʈ ��     ����
 1����Ʈ       0xxxxxxx
 2����Ʈ       1xxxxxxx 0xxxxxxx
 3����Ʈ       1xxxxxxx 1xxxxxxx 0xxxxxxx
 ...
 8����Ʈ       1xxxxxxx ... 0xxxxxxx
 9����Ʈ       1xxxxxxx (8��) xxxxxxxx => 64bits
 */

int main(int argc, char* argv[])
{
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    //cerr : error�� ��� �޽��� ��Ʈ��
    //unitbuf : �÷��ø� �ڵ����� ���� ���� ���� �̰� ������ �ڵ��÷���
    //�ݴ� -> std::nounitbuf -> �ڵ� �÷��� ��Ȱ��ȭ
    // endl vs \n ���� :�÷��ÿ��� endl�� buffer�� ��� -> �̰� ����� �� ���
    //https://torymur.github.io/sqlite-repr/ - ��� ���� �ľ��ϴ� �� ����

    if(argc!=3){
        std::cerr<<"Expected two arguments"<<std::endl;
        return 1;
    }

    std::string database_file_path = argv[1];
    std::string command = argv[2];

    if (command == ".dbinfo"){
        std::ifstream database_file(database_file_path, std::ios::binary);

        if(!database_file){
            std::cerr<<"Failed to open the database file"<<std::endl;
            return 1;
        }

        database_file.seekg(16);
        char buffer[2];
        database_file.read(buffer,2);

        unsigned short page_size = (static_cast<unsigned char>(buffer[1])|
                                    (static_cast<unsigned char>(buffer[0])<<8));
        std::cout<<"database page size:"<<page_size<<std::endl;
        //��Ʋ �ص������ �� ���� �ľ��ؾ��� (DB�� ��) �޿��� ��
        unsigned short table_num = get_num_tables(database_file);

        std::cout<<"database table num:"<<table_num<<std::endl;

        //���� �ð� ���̺� �̸� ������
        //�̰� ��ġ�� �����̿��� �� ã�ƾ���
    }
    else if (command == ".tables"){ //�̰� ������ ��ġ�� �ִ� �� �ƴ�

        std::ifstream database_file(database_file_path, std::ios::binary);

        if(!database_file){
            std::cerr<<"Failed to open the database file"<<std::endl;
            return 1;
        }

        unsigned short num_tables = get_num_tables(database_file);

        //sqlite header size = 0x64 = 100 bytes
        //Page header :8 bytes

        char buffer[512];
        for(int i=0;i<num_tables;++i){
            database_file.seekg(108+2*i); //header_size +pageheader
            database_file.read(buffer,2);
            unsigned short cell_offset = (static_cast<unsigned char>(buffer[1])|
                                    (static_cast<unsigned char>(buffer[0])<<8));

            database_file.seekg(cell_offset);

            uint64_t record_size = parse_varint(database_file);
            uint64_t row_id = parse_varint(database_file);
            uint64_t record_header_size = parse_varint(database_file);
            uint64_t type_len = (parse_varint(database_file)-13)/2;
            uint64_t name_len = (parse_varint(database_file)-13)/2;
            uint64_t tbl_name_len = (parse_varint(database_file)-13)/2;
            uint64_t root_page = (parse_varint(database_file));
            uint64_t sql = parse_varint(database_file);
            //�� ������ ���� ������ �ִ� ����

            database_file.ignore(type_len);
            database_file.ignore(name_len);
            std::string tbl_name(tbl_name_len, ' ');
            //char* buf = new char[tbl_name_len];
            //database_file.read(buf, tbl_name_len);
            database_file.read(&tbl_name[0], tbl_name_len); //database_file.read(tbl_name.data(),tbl_name_len); �� C++ 11 �̻󿡼��� �Ǵ� �ڵ���

            if(tbl_name == "sqlite_sequence"){
                continue;
            }

            std::cout << tbl_name<<' ';
        }
        std::cout << std::endl;
    }
    else
    {
        std::string lowercase_command = command;
        for(char& c :lowercase_command){
            c=std::tolower(c);
        }
        int select_idx = lowercase_command.find("select");
        int from_idx = lowercase_command.find("from");
        int where_idx = lowercase_command.find("where");

        std::vector<std::string> split_command = split(command, ' ');
        std::string target_table_name =
            where_idx == -1
                ? command.substr(from_idx + 5)
                : command.substr(from_idx + 5, where_idx - from_idx - 6);

        std::ifstream database_file(database_file_path, std::ios::binary);
        if (!database_file)
        {
            std::cerr << "Failed to open the database file" << std::endl;
            return 1;
        }

        unsigned short page_size = get_page_size(database_file);
        unsigned short num_tables = get_num_tables(database_file);

        char buffer[512];
        int target_root_page = -1;
        std::string create_statement;

        //superheroes.db "SELECT id, name FROM superheraes WHERE eye_color = 'Pink Eyes'
        //B-tree �ڽ� ����� �ִ� ���ڰ� �θ𺸴� ū��
        //sqlite�� b-tree�� �Ǿ�����

        for (int i = 0; i < num_tables; i++)
        {
            database_file.seekg(108 + 2 * i);
            database_file.read(buffer, 2);

            unsigned short cell_offset =
                (static_cast<unsigned char>(buffer[1]) |
                 (static_cast<unsigned char>(buffer[0]) << 8));
            database_file.seekg(cell_offset);

            uint64_t record_size = parse_varint(database_file);
            uint64_t row_id = parse_varint(database_file);
            uint64_t record_header_size = parse_varint(database_file);
            uint64_t type_len = (parse_varint(database_file) - 13) / 2;
            uint64_t name_len = (parse_varint(database_file) - 13) / 2;
            uint64_t tbl_name_len = (parse_varint(database_file) - 13) / 2;
            database_file.ignore(1);
            uint64_t sql_length = (parse_varint(database_file) - 13) / 2;
            database_file.ignore(type_len);
            database_file.ignore(name_len);

            std::string tbl_name(tbl_name_len, ' ');
            database_file.read(&tbl_name[0], tbl_name_len);

            if (tbl_name == target_table_name)
            {
                database_file.read(buffer, 1);
                target_root_page = static_cast<unsigned char>(buffer[0]);

                create_statement.resize(sql_length, '_');
                database_file.read(&create_statement[0], sql_length);

                break;
            }
        }

        if (target_root_page == -1)
        {
            std::cerr << "Failed to find table with name: " << target_table_name
                      << std::endl;
            return 1;
        }

        uint64_t base = (target_root_page - 1) * page_size;

        database_file.seekg(base + 3);
        database_file.read(buffer, 2);

        uint64_t count = (static_cast<unsigned char>(buffer[1]) |
                          (static_cast<unsigned char>(buffer[0]) << 8));

        if (split_command[1] == "count(*)" || split_command[1] == "COUNT(*)")
        {
            std::cout << count << std::endl;
        }
        else
        {
            std::string cols_string =
                command.substr(select_idx + 7, from_idx - select_idx - 8);
            std::vector<std::string> target_column_names =
                split(cols_string, ' ');

            for (int i = 0; i < target_column_names.size(); ++i)
            {
                if (target_column_names[i].back() == ',')
                {
                    target_column_names[i].pop_back();
                }
            }

            std::string where_column, where_arg;

            if (where_idx != -1)
            {
                std::vector<std::string> where_clause =
                    split(command.substr(where_idx + 6), '\'');
                where_column = where_clause[0].substr(0,where_clause[0].size()-3);
                where_arg = where_clause[1];
            }

            create_statement.pop_back();

            std::vector<std::string> columns = split(
                create_statement.substr(create_statement.find("(")+1), ',');
            std::vector<int> column_indices(target_column_names.size(), -1);
            int where_column_idx = -1;

            for (int i = 0; i < columns.size(); ++i)
            {
                std::vector<std::string> split_column = split(columns[i], ' ');

                for (std::string& s : split_column)
                {
                    std::string t = s;
                    t.erase(std::remove_if(t.begin(), t.end(), [](unsigned char ch){ return ch == '\n' || ch == '\t' || ch =='('; }), t.end());


                    if (!t.empty())
                    {
                        for (int j = 0; j < target_column_names.size(); ++j)
                        {
                            if (t == target_column_names[j])
                            {
                                column_indices[j] = i;
                            }
                        }

                        if (t == where_column)
                        {
                            where_column_idx = i;
                        }

                        break;
                    }
                }
            }

            for (int idx : column_indices)
            {
                if (idx == -1)
                {
                    std::cerr << "Failed to find column with name" << std::endl;
                    return 1;
                }
            }

            if (where_idx != -1 && where_column_idx == -1)
            {
                std::cerr << "Failed to find column with name" << std::endl;
                return 1;
            }

            if(traverse_btree(database_file,base,columns,column_indices,where_column_idx,where_idx,where_arg)!=0){
                std::cerr << "Error Traversing B-Tree" << std::endl;
                return 1;
            }
        }
    }

    return 0;
}
