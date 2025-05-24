#include <cstring>
#include <fstream>
#include <iostream>

//Database file
/*

header info
앞에 있는 한줄 정보 magic string
4글자 => Database page size

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

        database_file.seekg(103);
        char buffer2[2];
        database_file.read(buffer2,2);
        unsigned short table_num = (static_cast<unsigned char>(buffer2[1])|
                                    (static_cast<unsigned char>(buffer2[0])<<8));
        //DB 내부는 b-tree로 이뤄짐, 페이지 내 cell의 갯수 = table 갯수

        std::cout<<"database table num:"<<table_num<<std::endl;

        //다음 시간 테이블 이름 얻어오기
        //이건 위치가 동적이여서 잘 찾아야함
    }

    return 0;
}
