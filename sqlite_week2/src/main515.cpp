#include <cstring>
#include <fstream>
#include <vector>
#include <regex>
#include <iostream>

//Database file
/*

header info
앞에 있는 한줄 정보 magic string
4글자 => Database page size

*/
/*
5월 15일
SQLite엔 dbinfo말고도 다른 명령어가 있다
.tables
테이블 이름 출력
SQL Database Schema에 들어있음
tbl_name txt가 있다
Gtableoranges까지 접근해야함

1. sqlite schema page의 shell pointer 배열 찾자
2. 그기 offset이 있어서 table name 접근ㄱㄴ
3. schema는 table b tree에 들어있음
leaf shell은 3가지 type으로 구성
레코드 크기(varint), rowid(varint), record(record)
record =>header, body
header => header size(varint), record each row 직렬 type code(varint)
body => record row value(순서대로)

0ec3 -> 0f3c까지 봐야함
첫 바이트가 record size
세번쨰 바이트가 record header size
4 -> schema type
5 -> schema name
6 -> table name size
7 -> root page type
8 9 -> sql serial type

17을 보고
23 - 13 / 2 => 5
왜 13을?
(짝수 일땐 12
 sql

 6번째 byte에 가서 그 값 확인해서 -13 /2 하면
 위치가 나옴

 오늘 과제 :테이블에 있는 내 코드의 갯수를 출력하기
 sqlite.exe sample.db "SELECT COUNT(*) FROM apples' - apples만 읽으삼, ㅣ거 command
오늘 한 걸로 가능함

다음 시간에 : 저 뒤에 코드가 계속 바뀌는 걸 구현할거임
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
 바이트 수     구조
 1바이트       0xxxxxxx
 2바이트       1xxxxxxx 0xxxxxxx
 3바이트       1xxxxxxx 1xxxxxxx 0xxxxxxx
 ...
 8바이트       1xxxxxxx ... 0xxxxxxx
 9바이트       1xxxxxxx (8개) xxxxxxxx => 64bits
 */

int main(int argc, char* argv[])
{
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    //cerr : error용 출력 메시지 스트링
    //unitbuf : 플러시를 자동으로 할지 말지 지정 이걸 적으면 자동플러시
    //반대 -> std::nounitbuf -> 자동 플러시 비활성화
    // endl vs \n 차이 :플래시여부 endl은 buffer를 비움 -> 이게 비용이 들어서 비쌈
    //https://torymur.github.io/sqlite-repr/ - 헤더 정보 파악하는 데 좋음

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
        //리틀 앤디안인지 빅 인지 파악해야함 (DB는 빅) 왼에서 오
        unsigned short table_num = get_num_tables(database_file);

        std::cout<<"database table num:"<<table_num<<std::endl;

        //다음 시간 테이블 이름 얻어오기
        //이건 위치가 동적이여서 잘 찾아야함
    }
    else if (command == ".tables"){ //이건 정해진 위치에 있는 게 아님

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
            //이 다음이 실제 데이터 있는 영역

            database_file.ignore(type_len);
            database_file.ignore(name_len);
            std::string tbl_name(tbl_name_len, ' ');
            //char* buf = new char[tbl_name_len];
            //database_file.read(buf, tbl_name_len);
            database_file.read(&tbl_name[0], tbl_name_len); //database_file.read(tbl_name.data(),tbl_name_len); 이 C++ 11 이상에서만 되는 코드임

            if(tbl_name == "sqlite_sequence"){
                continue;
            }

            std::cout << tbl_name<<' ';
        }
        std::cout << std::endl;
    }
    return 0;
}
