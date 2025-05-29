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
    else{
        std::vector<std::string> split_command = split(command, ' ');
        std::string target_table_name = split_command.back();

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
            uint64_t sql_length = (parse_varint(database_file)-13)/2;
            database_file.ignore(type_len);
            database_file.ignore(name_len);

            std::string tbl_name(tbl_name_len, ' ');
            //char* buf = new char[tbl_name_len];
            //database_file.read(buf, tbl_name_len);
            database_file.read(&tbl_name[0], tbl_name_len); //database_file.read(tbl_name.data(),tbl_name_len); �� C++ 11 �̻󿡼��� �Ǵ� �ڵ���

            if (tbl_name == target_table_name)
            {
                database_file.read(buffer, 1);
                target_root_page = static_cast<unsigned char>(buffer[0]);

                create_statement.resize(sql_length,'_');

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

        uint64_t base = (target_root_page-1)*page_size;

        database_file.seekg(base + 3);
        database_file.read(buffer, 2);

        uint64_t count = (static_cast<unsigned char>(buffer[1]) |
                          (static_cast<unsigned char>(buffer[0]) << 8));
        if(split_command[1] == "count(*)" || split_command[1] == "COUNT(*)"){
            std::cout << count << std::endl;
        }
        else{
            std::string target_column_name = split_command[1];
            std::string table_name = split_command[3];
            //std::cout<< "Create statement = " << create_statement << '\n';
            create_statement.pop_back();//������ �� ������
            //15�� �� ����? CREATE TABLE �ڸ���?
            std::vector<std::string> columns = split(create_statement.substr(15+table_name.size()),',');
            int column_idx = -1;
            for (int i=0;i<columns.size(); ++i){
                std::vector<std::string> split_column = split(columns[i],' ');
                for(std::string& s: split_column){

                    std::string t = s;
                    t.erase(std::remove_if(t.begin(), t.end(),[](unsigned char ch){return ch=='\n'||ch=='\t';}),t.end());

                    if(t.size()>0){
                        if(t==target_column_name){
                            column_idx = i;
                        }
                        break;
                    }
                }
                if(column_idx != -1){
                    break;
                }
            }

            if(column_idx == -1){
                std::cerr << "Failed to find column with name: " << target_column_name << std::endl;
            }
            for(int i=0;i<count;++i){
                database_file.seekg(base+8+2*i);
                database_file.read(buffer,2);

                unsigned short cell_offset = (static_cast<unsigned char>(buffer[1])|
                                              (static_cast<unsigned char>(buffer[0])<<8));

                database_file.seekg(base+cell_offset);
                uint64_t record_size = parse_varint(database_file);
                uint64_t row_id = parse_varint(database_file);
                uint64_t record_header_size = parse_varint(database_file);

                std::vector<uint64_t> headers;

                for(int j=0;j<columns.size();++j){
                    headers.push_back(parse_varint(database_file));
                }

                std::vector<std::string> data;

                for(int j=0;j<columns.size();++j){
                    std::string temp;

                    if(headers[j]==0){

                    }
                    else if(headers[j]<=7){
                        uint64_t size = headers[j];
                        if(headers[j] == 5){
                            size = 6;
                        }
                        else if(headers[j]==6){
                            size = 8;
                        }

                        temp.resize(size, '_');
                        database_file.read(&temp[0], size);
                    }
                    else if(headers[j]>=12&&headers[j]%2==0){
                        uint64_t size = (headers[j]-12)/2;
                        temp.resize(size, '_');
                        database_file.read(&temp[0], size);
                    }
                    else if(headers[j]>=13&&headers[j]%2==1){
                        uint64_t size = (headers[j]-13)/2;
                        temp.resize(size, '_');
                        database_file.read(&temp[0], size);
                    }
                    else{
                        std::cerr << "Error parsing data" << std::endl;
                        return 1;
                    }
                    data.push_back(temp);
                }
                std::cout<<data[column_idx]<<std::endl;
            }
        }
    }

    return 0;
}
