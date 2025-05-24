#include <cstring>
#include <fstream>
#include <iostream>

//Database file
/*

header info
�տ� �ִ� ���� ���� magic string
4���� => Database page size

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

        database_file.seekg(103);
        char buffer2[2];
        database_file.read(buffer2,2);
        unsigned short table_num = (static_cast<unsigned char>(buffer2[1])|
                                    (static_cast<unsigned char>(buffer2[0])<<8));
        //DB ���δ� b-tree�� �̷���, ������ �� cell�� ���� = table ����

        std::cout<<"database table num:"<<table_num<<std::endl;

        //���� �ð� ���̺� �̸� ������
        //�̰� ��ġ�� �����̿��� �� ã�ƾ���
    }

    return 0;
}
