#include <iostream>
#include <string>
#include "test.pb.h"

int main(){
    test::LoginResponse rs1;
    test::ResultCode* rc1 = rs1.mutable_result();
    rc1->set_errcode(1);
    rc1->set_errmsg("login success");
    std::cout << rs1.success() << std::endl;

    test::GetFriendListsResponse rsp2;
    test::ResultCode* rc2 = rsp2.mutable_result();
    rc2->set_errcode(0);

    test::User* user1 = rsp2.add_friend_list();
    user1->set_name("zhang san");
    user1->set_age(20);
    user1->set_sex(test::User::MAN);

    test::User* user2 = rsp2.add_friend_list();
    user2->set_name("li si");
    user2->set_age(22);
    user2->set_sex(test::User::MAN);

    std::cout << rsp2.friend_list_size() << std::endl;

    return 0;
}
/*
int main1(){
    test::LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("123456");
    
    std::string send_str;
    if(req.SerializeToString(&send_str)){
        std::cout << send_str.c_str() << std::endl;
    }

    test::LoginRequest reqB;
    if(reqB.ParseFromString(send_str)){
        std::cout << send_str << std::endl;
        std::cout << reqB.name() << std::endl;
        std::cout << reqB.pwd() << std::endl;
    }

    return 0;
}
*/