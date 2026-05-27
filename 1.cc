#include <curl/curl.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cpp_redis/cpp_redis>
class verifycode {
    private:
     cpp_redis::client redis_;
    public:
     
     std::string code() {
         srand((unsigned int)time(NULL));
         std::string code;
         for (int i = 0; i < 7; i++) {
             code += ((char)('0' + (rand() % 10)));
         }
         return code;
     }
     void addredis(const std::string& code,const std::string& count) {
         redis_.connect("127.0.0.1", 6379);
         redis_.setex(count, 300, code);
         redis_.sync_commit();
     }
     bool checkcode(const std::string &inputstring,const std::string&count){
         std::string truecode;
         redis_.get(count, [&](cpp_redis::reply& reply) {
             if (reply.is_string()) {
                 truecode = reply.as_string();
             }
         });
         redis_.sync_commit();
         if(inputstring==truecode){
             return true;
         }else{
             return false;
         }
     }
     bool sendcom(const std::string& servercount,const std::string&clientcount,const std::string& code) { 
        curl_global_init(CURL_GLOBAL_ALL); 
        CURL* curl=curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, "smtp.qq.com:465");
        curl_easy_setopt(curl, CURLOPT_USERNAME, servercount);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "irukixpmtzcwchcf");
        curl_easy_setopt(curl,CURLOPT_MAIL_FROM,servercount);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, clientcount);
        std::string data = "验证码：" + code + "(该验证码将在5分钟之后过期)";
        curl_easy_setopt(curl, CURLOPT_READDATA, data.c_str());
        curl_easy_perform(curl);
        long num;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &num);
        if(num==250){
            std::cout << "发送成功";
        }else{
            std::cout << "发送失败";
        }
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return true;
     }
};
int main(){
    verifycode C;
    std::string code=C.code();
    C.addredis(code, "3541053286@qq.com");
    C.sendcom("3541053286@qq.com", "3541053286@qq.com", code);
    std::string s;
    std::cin >> s;
    bool ret = C.checkcode(s, "3541053286@qq.com");
    std::cout << "login" << ret;
    return 0;
}