#include <curl/curl.h>
#include <cpp_redis/cpp_redis>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
size_t mail_payload(void* ptr, size_t size, size_t nmemb, void* userp) {
    std::string* data = (std::string*)userp;
    size_t len = data->size();
    if (len <= 0){
        return 0;
    }
    memcpy(ptr, data->c_str(), len);
    data->clear();  
    return len;
}
class verifycode {
   private:
    cpp_redis::client redis_;

   public:
    verifycode() {
        srand(time(NULL));
        redis_.connect("127.0.0.1", 6379);
    }
    std::string code() {
        std::string code;
        for (int i = 0; i < 4; i++) {
            code += ('0' + rand() % 10);
        }
        return code;
    }
    void addredis(const std::string& code, const std::string& account) {
        redis_.setex(account, 300, code);
        redis_.sync_commit();
    }
    void checkcode(const std::string& inputcode, const std::string& account) {
        std::string truecode;
        redis_.get(account, [&](cpp_redis::reply& reply) {
            if (reply.is_string())
                truecode = reply.as_string();
        });
        redis_.sync_commit();

        if (inputcode == truecode) {
            std::cout << "注册成功" << std::endl;
        } else {
            std::cout << "验证码错误" << std::endl;
        }
    }
    void sendcom(const std::string& serveraccount,
                 const std::string& clientaccount,
                 const std::string& code) {
        CURL* curl = curl_easy_init();
        std::string from = "<" + serveraccount + ">";
        std::string to = "<" + clientaccount + ">";
        std::string mail = "To: " + clientaccount +
                           "\r\n"
                           "From: " +
                           serveraccount +
                           "\r\n"
                           "Subject: 验证码\r\n"
                           "\r\n"
                           "您的验证码是：" +
                           code +
                           "\r\n"
                           "5分钟内有效\r\n";
        struct curl_slist* recipients = NULL;
        recipients = curl_slist_append(recipients, to.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.qq.com:465");
        curl_easy_setopt(curl, CURLOPT_USERNAME, serveraccount.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "miojajsaujebdbch");
        // curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=PLAIN");
        curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, mail_payload);
        curl_easy_setopt(curl, CURLOPT_READDATA, &mail);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            std::cout << "发送成功！" << std::endl;
        } else {
            std::cout << "发送失败: " << curl_easy_strerror(res) << std::endl;
        }
        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }
};
int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    verifycode c;
    std::string server = "3541053286@qq.com";
    std::string client = "3541053286@qq.com";
    std::string code = c.code();
    c.addredis(code, client);
    c.sendcom(server, client, code);
    std::cout << "请输入验证码:";
    std::string input;
    std::cin >> input;
    c.checkcode(input, client);
    curl_global_cleanup();
    return 0;
}
