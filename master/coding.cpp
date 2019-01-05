#include <string>

void EncodeString(const std::string &str, std::string &ret){
        int strlen = str.size();
        ret.append("$");
        ret.append(std::to_string(strlen));
        ret.append("\r\n");
        ret.append(str);
        ret.append("\r\n ");
}
void EncodeInt(int val, std::string &ret){
        std::string str = std::to_string(val);
        ret.append("$");
        ret.append(std::to_string(str.size()));
        ret.append("\r\n");
        ret.append(str);
        ret.append("\r\n ");
}

