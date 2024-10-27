/*
    Potential Bugs:
    The email_list cannot handle spaces I think
*/


/*
    To run
    g++ -std=c++11 -o smart_mailer smart_mailer.cpp -lboost_system -lboost_filesystem -lboost_thread -lssl -lcrypto -lpthread


    To install
    sudo dnf install gcc-c++ boost-devel openssl-devel
    or
    sudo apt-get update
    sudo apt-get install build-essential libboost-all-dev libssl-dev


*/



#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <map>


#include <thread>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <iostream>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>


const std::string EMAIL = "maxchewjw@gmail.com";
//const std::string PASSWORD = "Qweasd6969!!!"; 
const std::string PASSWORD = "nukr hzaa qznv jwro";

const std::string host = "192.168.1.6"; // Replace with your server's IP or domain
const std::string port = "8080";                     // Replace with your server's port if different

const std::string EMAIL_TEMPLATE_TXT = "email_template.txt";

const std::string MAIL_CSV_PATH = "email_list.csv";

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = net::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;


struct Recipient  
{
    std::string department_code;
    std::string email;
    std::string name;
};






void read_csv(const std::string& filename, std::vector<Recipient>& recipients)
{
    std::ifstream file(filename);
    if (!file.is_open()) 
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::string line;
    // Optionally skip header line if your CSV has headers
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string department_code, email, name;

        // Assuming the CSV columns are: department_code,email,name
        if 
        (
            std::getline(ss, department_code, ',') &&
            std::getline(ss, email, ',') &&
            std::getline(ss, name)
        ) 
        {

            Recipient recipient;
            recipient.department_code = department_code;
            recipient.email = email;
            recipient.name = name;

            recipients.push_back(recipient);
        }
    }
    file.close();
}

/*
    Function to replace the #name# and #department# with the recipient details
*/
void replace_placeholders(std::string& text, const Recipient& recipient) 
{
    size_t pos;

    while ((pos = text.find("#name#")) != std::string::npos) {
        text.replace(pos, 6, recipient.name);
    }

    while ((pos = text.find("#department#")) != std::string::npos) {
        text.replace(pos, 12, recipient.department_code);
    }

    // Add more placeholders if needed
}



std::string email_subject;
std::string email_body;

void read_email_template(const std::string& filename, std::string& subject, std::string& body) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening email template file: " << filename << std::endl;
        return;
    }

    // Read the first line as subject
    std::getline(file, subject);

    // Read the rest as body
    std::stringstream buffer;
    buffer << file.rdbuf();
    body = buffer.str();

    file.close();
}



void get_count() {
    try {
        std::string target = "/counter";
        int version = 11; // HTTP/1.1

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        tcp::socket socket(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        net::connect(socket, results.begin(), results.end());

        // Set up an HTTP GET request message
        http::request<http::empty_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(socket, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(socket, buffer, res);

        // Write the message to stdout
        std::cout << res << std::endl;

        // Gracefully close the socket
        beast::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

    } catch (std::exception const& e) {
        std::cerr << "Error during get_count: " << e.what() << std::endl;
    }
}






/*
    Socket: TCP socket wrapped in a SSL_socket for encryption
    cmd: command that we are sending to the SMTP server


    boost::asio:write ->    blocking write function that writes to socket the data in the buffer
    boost::asio:buffer ->   converts data given into a format of boost::asio::const_buffer object
                            to be used using Boost.Asio’s socket functions
*/
void send_command(ssl::stream<tcp::socket>& socket, const std::string& cmd) 
{
    boost::asio::write(socket, boost::asio::buffer(cmd));
}


/*
    Reads one line of SMTP response, bcs each line ends with "\r\n"

    boost::asio::streambuf -> buffer type specifically designed for stream-oriented I/O
    std::istream -> input stream class to handle input streams
*/
std::string read_response(ssl::stream<tcp::socket>& socket) 
{
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");
    std::istream response_stream(&response);
    std::string response_line;
    std::getline(response_stream, response_line);
    return response_line;
}

/*
    Encodes given string into Base64 format

    Required as Base64 is used in SMTP for encoding credentials

    boost::beast::detail::base64::encoded_size(input.size()) -> calculates the required size of the encoded output based on the input size
    boost::beast::detail::base64::encode -> Boost function that performs the Base64 encoding
*/
std::string base64_encode(const std::string& input) 
{
    std::string output;
    output.resize(boost::beast::detail::base64::encoded_size(input.size()));
    boost::beast::detail::base64::encode(&output[0], input.data(), input.size());
    return output;
}



void send_email(ssl::stream<tcp::socket>& ssl_socket, const std::string& recipient_email,
                const std::string& subject, const std::string& body) 
{

    std::string response;

    // MAIL FROM
    std::string mail_from = "MAIL FROM:<" + std::string(EMAIL) + ">\r\n";
    send_command(ssl_socket, mail_from);
    response = read_response(ssl_socket);
    std::cout << "Server: " << response << std::endl;

    // if (response.substr(0, 3) != "250") 
    // {
    //     std::cerr << "Error after MAIL FROM: " << response << std::endl;
    //     return;
    // }

    // RCPT TO
    std::string rcpt_to = "RCPT TO:<" + recipient_email + ">\r\n";
    send_command(ssl_socket, rcpt_to);
    response = read_response(ssl_socket);
    std::cout << "Server: " << response << std::endl;

    // if (response.substr(0, 3) != "250") 
    // {
    //     std::cerr << "Error after RCPT TO: " << response << std::endl;
    //     return;
    // }

    // DATA
    send_command(ssl_socket, "DATA\r\n");
    response = read_response(ssl_socket);
    std::cout << "Server: " << response << std::endl;

    // if (response.substr(0, 3) != "354") 
    // {
    //     std::cerr << "Error after DATA: " << response << std::endl;
    //     return;
    // }

    // Email headers and content
    std::string from = "From: Your Name <" + std::string(EMAIL) + ">\r\n";
    std::string to = "To: <" + recipient_email + ">\r\n";
    std::string content_type = "Content-Type: text/html; charset=UTF-8\r\n";
    std::string mime_version = "MIME-Version: 1.0\r\n";

    // Wrap the body in HTML tags
    std::string html_body = "<html><body>" + body + "</body></html>";

    std::string email_content = "Subject: " + subject + "\r\n" + from + to + mime_version + content_type + "\r\n" + body + "\r\n.\r\n";

    // Send the email content
    send_command(ssl_socket, email_content);

    response = read_response(ssl_socket);
    std::cout << "Server: " << response << std::endl;

    // if (response.substr(0, 3) != "250") 
    // {
    //     std::cerr << "Error after sending email content: " << response << std::endl;
    //     return;
    // }
}





void send_emails() 
{
    try 
    {
        /*
            Section 1: Creating SSL context/sockets, resolving SMTP domains
        */
    
        // Create SSL context and socket
        // Creating the SSL context that defines the set of parameters we want 
        // for using SSL/TLS protocols 
        boost::asio::io_context io_context;
        ssl::context ctx(ssl::context::tlsv12_client);
        ssl::stream<tcp::socket> ssl_socket(io_context, ctx);

        // tells SSL context (the ctx) to use system's default locations for trusted root certificate
        ctx.set_default_verify_paths();

        // Resolve SMTP server and connect
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("smtp.gmail.com", "465");

        // we use ssl_socket.next_layer() to access the underlying TCP socket wrapped in SSL Socket
        // connect() here is us trying to connect to any of the resolved IP endpoints that we have resolved 
        // from .resolve() resolving our smtp server
        boost::asio::connect(ssl_socket.next_layer(), endpoints);

        // Perform SSL handshake
        ssl_socket.handshake(ssl::stream_base::client);







        /*
            Section 2: SMTP connection and authentication
        */ 
        

        // Send EHLO
        std::string ehlo_command = "EHLO gmail.com\r\n";
        send_command(ssl_socket, ehlo_command);
        
        std::string response = read_response(ssl_socket);
        std::cout << "Server: " << response << std::endl;


        /*
            while() loop, loops through and processes every line of response by server 
            This is done by checking whether the fourth character is '-'

            An example of a multi-line response is
            250-smtp.gmail.com at your service
            250-AUTH LOGIN PLAIN
            250 SIZE 35882577

            response[3] checks for the '-' as the fourth character.
            The last line does not have this indicating the end of the server response

        */
        while (response[3] == '-') 
        {
            response = read_response(ssl_socket);
            std::cout << "Server: " << response << std::endl;
        }

        // Authentication
        send_command(ssl_socket, "AUTH LOGIN\r\n");
        response = read_response(ssl_socket);
        std::cout << "Server: " << response << std::endl;

        // Send encoded_email
        std::string encoded_email= base64_encode(EMAIL) + "\r\n";
        send_command(ssl_socket, encoded_email);
        response = read_response(ssl_socket);
        std::cout << "Server: " << response << std::endl;

        // Send encoded password
        std::string encoded_password = base64_encode(PASSWORD) + "\r\n";
        send_command(ssl_socket, encoded_password);
        response = read_response(ssl_socket);
        std::cout << "Server: " << response << std::endl;
    
        // Adding this make the thing fail
        // // Check if authentication was successful
        // // This prevents the program from moving on till a response comes in 
        // if (response.substr(0, 3) != "235") 
        // {
        //     std::cerr << "Authentication failed: " << response << std::endl;
        //     return;
        // }


        // Prompt user for department code
        std::string department_code;
        std::cout << "Enter the department code (or 'all' to send to all departments): ";
        std::getline(std::cin, department_code);

        // Read the CSV file
        std::vector<Recipient> recipients;
        read_csv(MAIL_CSV_PATH, recipients);

        // Filter recipients based on department code
        std::vector<Recipient> filtered_recipients;
        for (const auto& recipient : recipients) 
        {
            if (department_code == "all" || recipient.department_code == department_code) 
            {
                filtered_recipients.push_back(recipient);
            }
        }

        if (filtered_recipients.empty()) 
        {
            std::cout << "No recipients found for the specified department code.\n";
            return;
        }

        // Read email template
        std::string email_subject;
        std::string email_body;
        read_email_template(EMAIL_TEMPLATE_TXT, email_subject, email_body);

        // Map to keep track of emails sent per department
        std::map<std::string, int> department_counts;

        // Loop through each recipient and send email
        for (const auto& recipient : filtered_recipients) 
        {
            // Personalize subject and body
            std::string personalized_subject = email_subject;
            std::string personalized_body = email_body;

            replace_placeholders(personalized_subject, recipient);
            replace_placeholders(personalized_body, recipient);

            // Include tracking pixel
            std::string tracking_pixel = "<img src=\"http://" + host + ":" + port + "/image\" alt=\"image\" width=\"1\" height=\"1\" />";

            



            // Append or insert the tracking pixel into the email body
            personalized_body += tracking_pixel;

            // Send the email using your SMTP code
            send_email(ssl_socket, recipient.email, personalized_subject, personalized_body);

            // Update department count
            department_counts[recipient.department_code]++;

            // Introduce a delay between emails to reduce spam
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Generate report
        std::cout << "\nEmail sending report:\n";
        for (const auto& entry : department_counts) {
            std::cout << "Department " << entry.first << ": " << entry.second << " emails sent.\n";
        }

        // QUIT command to end SMTP session
        send_command(ssl_socket, "QUIT\r\n");
        std::string response_quit = read_response(ssl_socket);
        std::cout << "Server: " << response_quit << std::endl;

    } catch (std::exception& e) 
    {
        std::cerr << "Exception during send_emails: " << e.what() << std::endl;
    }
}







int main() {
    while (true) {
        std::string command;
        std::cout << "\nEnter a command ('send' or 'get_count', or 'exit' to quit): ";
        std::getline(std::cin, command);

        if (command == "send") {
            // Call the send function
            send_emails();
        } else if (command == "get_count") {
            // Call the get_count function
            get_count();
        } else if (command == "exit") {
            std::cout << "Exiting program.\n";
            break;
        } else {
            std::cout << "Invalid command. Please enter 'send', 'get_count', or 'exit'.\n";
        }
    }
    return 0;
}
