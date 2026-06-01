#include <curl/curl.h>
#include <termios.h>
#include <unistd.h>
#include <algorithm>
#include <cpp_redis/cpp_redis>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <future>
#include <iostream>
#include <string>
class Friend{
    private:
     cpp_redis::client redis_;
     std::vector<std::string> friendlist1;
     std::vector<std::string> friendlist2;
     std::vector<std::string> blocklist1;
     std::vector<std::string> blocklist2;
     std::vector<std::string> applylist1;
     std::vector<std::string> applylist2;
     std::vector<std::string> appliedlist1;
     std::vector<std::string> appliedlist2;
     std::vector<std::string> refuseaccount1;
     std::vector<std::string> refuseaccount2;
     std::vector<std::string> refusedaccount1;
     std::vector<std::string> refusedaccount2;

    public:
     Friend() { 
        redis_.connect("127.0.0.1", 6379);
     }
    bool addapply(std::string applyaccount,std::string appliedaccount){
        auto fut1 = redis_.exists({applyaccount});
        int num1 = fut1.get().as_integer();
        if(num1){
            applylist1.push_back(appliedaccount);
            appliedlist2.push_back(applyaccount);
            redis_.sadd("apply" + applyaccount, applylist1);
            redis_.sadd("applied" + appliedaccount, appliedlist2);
        } else {
        }
    }
    bool agreeapply(std::string applyaccount,std::string appliedaccount){
        friendlist1.push_back(appliedaccount);
        friendlist2.push_back(applyaccount);
        redis_.sadd("friend" + applyaccount, friendlist1);
        redis_.sadd("friend" + appliedaccount, friendlist2);
    }
    bool refuseapply(std::string applyaccount, std::string appliedaccount){
        refuseaccount1.push_back(applyaccount);
        refusedaccount2.push_back(appliedaccount);
        redis_.srem("applied" + appliedaccount, refuseaccount1);
        redis_.srem("apply" + applyaccount, refusedaccount2);
    }
    bool block(std::string applyaccount, std::string appliedaccount){
        blocklist1.push_back(appliedaccount);
        blocklist2.push_back(applyaccount);
        redis_.sadd("blockapply" + applyaccount, blocklist1);
        redis_.sadd("blockapplied" + appliedaccount, blocklist2);
    }
    bool cancleblock(std::string applyaccount, std::string appliedaccount){

    }
    bool delfriend(std::string applyaccount, std::string appliedaccount){

    }
    bool isfriend(std::string applyaccount, std::string appliedaccount){

    }
    bool isblock(std::string applyaccount, std::string appliedaccount){

    }
    bool onlien(std::string account){

    }
    std::vector<std::string> friendlist(std::string account){

    }
    std::vector<std::string> onlienlist(std::string acount){

    }
};