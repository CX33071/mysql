#include <curl/curl.h>
#include <cpp_redis/cpp_redis>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <future>
#include <iostream>
#include <string>

// 邮件发送回调
size_t mail_payload(void* ptr, size_t size, size_t nmemb, void* userp) {
    std::string* data = (std::string*)userp;
    size_t len = data->size();
    memcpy(ptr, data->c_str(), len);
    data->clear();
    return len;
}

class AccountManager {
   private:
    cpp_redis::client redis_;

    std::string generateCode() {
        std::string code;
        for (int i = 0; i < 4; ++i) {
            code += ('0' + rand() % 10);
        }
        return code;
    }

    void sendEmail(const std::string& serverAccount,
                   const std::string& clientAccount,
                   const std::string& subject,
                   const std::string& body) {
        CURL* curl = curl_easy_init();
        if (!curl)
            return;

        std::string from = "<" + serverAccount + ">";
        std::string to = "<" + clientAccount + ">";
        std::string mail = "To: " + clientAccount +
                           "\r\n"
                           "From: " +
                           serverAccount +
                           "\r\n"
                           "Subject: " +
                           subject +
                           "\r\n"
                           "\r\n" +
                           body;

        struct curl_slist* recipients = nullptr;
        recipients = curl_slist_append(recipients, to.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.qq.com:465");
        curl_easy_setopt(curl, CURLOPT_USERNAME, serverAccount.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD,
                         "miojajsaujebdbch");  // 替换为您的授权码
        curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, mail_payload);
        curl_easy_setopt(curl, CURLOPT_READDATA, &mail);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cout << "邮件发送失败: " << curl_easy_strerror(res)
                      << std::endl;
        } else {
            std::cout << "邮件已发送" << std::endl;
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }

   public:
    AccountManager() {
        srand(time(nullptr));
        redis_.connect("127.0.0.1", 6379);
    }

    void signup() {
        std::string account, password, confirm;
        std::cout << "请输入您的QQ邮箱: ";
        std::cin >> account;

        auto fut = redis_.exists({"user:" + account});
        redis_.sync_commit();
        if (fut.get().as_integer() == 1) {
            std::cout << "该账号已被使用，请直接登录" << std::endl;
            return;
        }

        std::cout << "请设置密码: ";
        std::cin >> password;
        std::cout << "请再次输入密码: ";
        std::cin >> confirm;
        if (password != confirm) {
            std::cout << "两次密码不一致，注册失败" << std::endl;
            return;
        }

        redis_.set("user:" + account, password);
        redis_.sync_commit();
        std::cout << "注册成功！" << std::endl;
    }

    void sendCaptcha(const std::string& serverAccount,
                     const std::string& email) {
        std::string code = generateCode();
        std::string key = "captcha:" + email;
        redis_.setex(key, 300, code);
        redis_.sync_commit();
        sendEmail(serverAccount, email, "聊天室验证码",
                  "您的验证码是：" + code + "，5分钟内有效。");
    }

    bool verifyCaptcha(const std::string& email, const std::string& inputCode) {
        auto fut = redis_.get("captcha:" + email);
        redis_.sync_commit();
        auto reply = fut.get();
        if (!reply.is_string())
            return false;
        if (reply.as_string() == inputCode) {
            redis_.del({"captcha:" + email});
            redis_.sync_commit();
            return true;
        }
        return false;
    }

    void loginWithPassword() {
        std::string account, password;
        std::cout << "请输入账号(QQ邮箱): ";
        std::cin >> account;
        std::cout << "请输入密码: ";
        std::cin >> password;

        auto fut = redis_.get("user:" + account);
        redis_.sync_commit();
        auto reply = fut.get();
        if (!reply.is_string()) {
            std::cout << "账号不存在，请先注册" << std::endl;
            return;
        }
        if (reply.as_string() == password) {
            std::cout << "登录成功！" << std::endl;
        } else {
            std::cout << "密码错误" << std::endl;
        }
    }

    void loginWithCaptcha(const std::string& serverAccount) {
        std::string email;
        std::cout << "请输入账号(QQ邮箱): ";
        std::cin >> email;

        auto fut = redis_.exists({"user:" + email});
        redis_.sync_commit();
        if (fut.get().as_integer() == 0) {
            std::cout << "账号不存在，请先注册" << std::endl;
            return;
        }

        sendCaptcha(serverAccount, email);
        std::cout << "请输入收到的验证码: ";
        std::string code;
        std::cin >> code;

        if (verifyCaptcha(email, code)) {
            std::cout << "登录成功！" << std::endl;
        } else {
            std::cout << "验证码错误或已过期" << std::endl;
        }
    }

    // 【修改点】忘记密码：发送原密码到邮箱（不重置）
    void forgotPassword(const std::string& serverAccount) {
        std::string email;
        std::cout << "请输入您的QQ邮箱: ";
        std::cin >> email;

        // 检查账号是否存在
        auto fut = redis_.exists({"user:" + email});
        redis_.sync_commit();
        if (fut.get().as_integer() == 0) {
            std::cout << "账号不存在" << std::endl;
            return;
        }

        // 发送验证码
        sendCaptcha(serverAccount, email);
        std::cout << "请输入收到的验证码: ";
        std::string code;
        std::cin >> code;

        // 验证验证码
        if (!verifyCaptcha(email, code)) {
            std::cout << "验证码错误或已过期，无法获取密码" << std::endl;
            return;
        }

        // 从 Redis 读取原密码（明文）
        auto pwd_fut = redis_.get("user:" + email);
        redis_.sync_commit();
        auto pwd_reply = pwd_fut.get();
        if (!pwd_reply.is_string()) {
            std::cout << "密码读取失败" << std::endl;
            return;
        }
        std::string originalPassword = pwd_reply.as_string();

        // 发送原密码到用户邮箱
        std::string mailBody = "您账号 " + email + " 的密码是：" +
                               originalPassword + "\n请妥善保管。";
        sendEmail(serverAccount, email, "【密码找回】您的账号密码", mailBody);
        std::cout << "原密码已发送到您的邮箱，请查收" << std::endl;
    }

    void destroyAccount() {
        std::string email, password;
        std::cout << "请输入要注销的账号: ";
        std::cin >> email;

        auto fut = redis_.get("user:" + email);
        redis_.sync_commit();
        auto reply = fut.get();
        if (!reply.is_string()) {
            std::cout << "账号不存在" << std::endl;
            return;
        }

        std::cout << "请输入密码确认注销: ";
        std::cin >> password;
        if (reply.as_string() != password) {
            std::cout << "密码错误，注销失败" << std::endl;
            return;
        }

        redis_.del({"user:" + email});
        redis_.sync_commit();
        std::cout << "账号已注销" << std::endl;
    }
};

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    AccountManager am;
    std::string serverEmail = "3541053286@qq.com";  // 替换为您的发件邮箱

    int choice;
    while (true) {
        std::cout << "\n========== 账号管理 ==========\n";
        std::cout << "1. 注册\n";
        std::cout << "2. 密码登录\n";
        std::cout << "3. 验证码登录\n";
        std::cout << "4. 忘记密码（发送原密码到邮箱）\n";
        std::cout << "5. 注销账号\n";
        std::cout << "0. 退出\n";
        std::cout << "请选择: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                am.signup();
                break;
            case 2:
                am.loginWithPassword();
                break;
            case 3:
                am.loginWithCaptcha(serverEmail);
                break;
            case 4:
                am.forgotPassword(serverEmail);
                break;
            case 5:
                am.destroyAccount();
                break;
            case 0:
                std::cout << "再见" << std::endl;
                curl_global_cleanup();
                return 0;
            default:
                std::cout << "无效选择" << std::endl;
        }
    }
}