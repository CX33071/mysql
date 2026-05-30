#include <curl/curl.h>
#include <cpp_redis/cpp_redis>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <future>
#include <iostream>
#include <string>
size_t mail_payload(void* ptr, size_t size, size_t nmemb, void* userp) {
    std::string* data = (std::string*)userp;
    size_t len = data->size();
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
        std::string account1 = account + "1";
        redis_.setex(account1, 300, code);
        redis_.sync_commit();
    }
    void signup(std::string& account, std::string& key) {
        std::cout << "请输入您的qq邮箱:";
        std::cin >> account;
        auto fut = redis_.exists({account});
        redis_.sync_commit();
        int exists = fut.get().as_integer();
        while (exists) {
            std::cout << "该账号已被使用，请重新输入帐号" << std::endl;
            std::cin >> account;
            auto fut = redis_.exists({account});
            redis_.sync_commit();
            exists = fut.get().as_integer();
        }
        std::cout << "请设置您的密码:";
        std::cin >> key;
        std::cout << "请再次输入密码:";
        std::string key1;
        std::cin >> key1;
        while (key != key1) {
            std::cout << "两次密码设置不一致,请再次设置您的密码:";
            std::cin >> key;
            std::cout << "请再次输入密码:";
            std::cin >> key1;
        }
        redis_.setnx(account, key);
        std::cout << "注册成功！";
    }
    void signin(const std::string server) {
        std::cout << "请选择：1验证码登录/2密码登录" << std::endl;
        int options;
        std::cin >> options;
        if (options == 1) {
            std::cout << "请输入您的qq邮箱帐号:";
            std::string qqaccount;
            std::cin >> qqaccount;
            verifycodesignin(server, qqaccount);
            std::cout << "登录成功";
        } else {
            std::cout << "请输入账号:";
            std::string account;
            std::cin >> account;
            std::string key;
            std::cout << "请输入密码:";
            std::cin >> key;
            std::string truekey;
            auto fut = redis_.get({account});
            redis_.sync_commit();
            truekey = fut.get().as_string();
            while (truekey != key) {
                std::cout << "密码错误，请选择1忘记密码/2重新输入密码："
                          << std::endl;
                int option;
                std::cin >> option;
                if (option == 1) {
                    getselfcode(server);
                }
                std::cout << "请输入密码:";
                std::cin >> key;
            }
            std::cout << "登录成功！";
        }
    }
    void destory(std::string account, const std::string& server) {
        std::string truekey;
        auto fut = redis_.get({account});
        redis_.sync_commit();
        truekey = fut.get().as_string();
        std::string key;
        std::cout << "注销需要密码，请选择：密码注销y/忘记密码n ";
        char c;
        std::cin >> c;
        if (c == 'n') {
            getselfcode(server);
        }
        std::cout << "请输入密码:";
        std::cin >> key;
        while (truekey != key) {
            std::cout << "密码错误，请重新输入密码:";
            std::cin >> key;
        }
        redis_.del({account});
        std::cout << "注销成功！" << std::endl;
    }
    void getselfcode(const std::string& server) {
        std::cout << "请输入您的qq邮箱帐号:";
        std::string qqaccount;
        std::cin >> qqaccount;
        verifycodesignin(server, qqaccount);
        std::string truecode;
        auto fut = redis_.get(qqaccount);
        redis_.sync_commit();
        truecode = fut.get().as_string();
        if (truecode.empty()) {
            std::cout << "密码获取失败";
        }
        std::string a = "***";
        sendcom(server, qqaccount, a, truecode);
        std::cout << "已经发送原密码到您邮箱";
    }
    void sendcom(const std::string& serveraccount,
                 const std::string& clientaccount,
                 const std::string& subject,
                 std::string s) {
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
        if (!s.empty()) {
            mail = "To: " + clientaccount +
                   "\r\n"
                   "From: " +
                   serveraccount +
                   "\r\n"
                   "Subject: 密码\r\n"
                   "\r\n"
                   "您的帐号密码是：" +
                   s + "\r\n";
        }
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
    void verifycodesignin(const std::string& server,
                          const std::string& qqaccount) {
        std::string s1 = code();
        addredis(s1, qqaccount);
        sendcom(server, qqaccount, s1, "");
        std::cout << "请输入验证码:";
        std::string inputcode;
        std::cin >> inputcode;
        std::string truecode;
        std::string account1;
        account1 = qqaccount + "1";
        auto fut = redis_.get({account1});
        redis_.sync_commit();
        truecode = fut.get().as_string();
        redis_.sync_commit();
        while (inputcode != truecode) {
            redis_.del({account1});
            std::cout << "验证码错误，已为您重新发送验证码";
            std::string s = code();
            redis_.setex(account1, 300, s);
            redis_.sync_commit();
            sendcom(server, qqaccount, s, "");
            auto fut = redis_.get({account1});
            redis_.sync_commit();
            truecode = fut.get().as_string();
            std::cout << "请输入新的验证码:";
            std::cin >> inputcode;
        }
        std::cout << "验证码正确" << std::endl;
    }
};

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    std::string account;
    std::string key;
    std::string server = "3541053286@qq.com";
    verifycode c;
    // c.signup(account,key);
    c.signin(server);
    c.destory(account, server);
    curl_global_cleanup();
    return 0;
}
