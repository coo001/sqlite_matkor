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
    return 0;
}
