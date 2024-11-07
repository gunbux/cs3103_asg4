/*
    Potential Bugs:
    The email_list cannot handle spaces I think
*/

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <iostream>

// SMTP Server Details
const std::string SMTP_SERVER = std::getenv("SMTP_SERVER") ? std::getenv("SMTP_SERVER") : "";
const std::string SMTP_DOMAIN = std::getenv("SMTP_DOMAIN") ? std::getenv("SMTP_DOMAIN") : "";
const std::string EMAIL = std::getenv("EMAIL") ? std::getenv("EMAIL") : "";
const std::string PASSWORD = std::getenv("PASSWORD") ? std::getenv("PASSWORD") : "";

// Pixel Tracking Server Details
const std::string pixel_server_ip = "pixel.chunyu.sh";
const std::string pixel_server_port = "443";

const std::string DEFAULT_EMAIL_TEMPLATE = "/app/assets/email_template.txt";

const std::string DEFAULT_MAIL_CSV_PATH = "/app/assets/email_list.csv";
const std::string DEFAULT_DEPARTMENT_CODE = "all"; // Replace with your default department code
// Boost namespaces
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

struct Recipient {
    std::string department_code;
    std::string email;
    std::string name;
};

/*
    Function to generate a random query parameter to prevent caching

    std::rand() -> generates a random number
    std::time(nullptr) -> gets the current time in seconds since the epoch
*/
std::string generate_random_query() {
    std::srand(std::time(nullptr));
    int random_number = std::rand();
    return "rand=" + std::to_string(random_number);
}

/*
 * Function to read the CSV file and populate the recipients vector
 *
 * filename: path to the CSV file
 * recipients: vector to store the recipient details
 */
void read_csv(const std::string &filename, std::vector<Recipient> &recipients) {
    std::ifstream file(filename);
    if (!file.is_open()) {
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
                ) {

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

    text: email template text
    recipient: recipient details
*/
void replace_placeholders(std::string &text, const Recipient &recipient) {
    size_t pos;

    while ((pos = text.find("#name#")) != std::string::npos) {
        text.replace(pos, 6, recipient.name);
    }

    while ((pos = text.find("#department#")) != std::string::npos) {
        text.replace(pos, 12, recipient.department_code);
    }
}

std::string email_subject;
std::string email_body;

void read_email_template(const std::string &filename, std::string &subject, std::string &body) {
    std::ifstream file(filename);
    //std::istringstream file_stream(file_content);  // Use string stream to read content

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

    // // Read the first line as subject
    // std::getline(file_stream, subject);

    // // Read the rest as body
    // std::stringstream buffer;
    // buffer << file_stream.rdbuf();
    // body = buffer.str();
}

void get_count() {
    try {
        std::string target = "/counter";
        int version = 11; // HTTP/1.1

        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx(ssl::context::tlsv12_client);

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        ssl::stream<tcp::socket> ssl_socket(ioc, ctx);

        // Look up the domain name
        auto const results = resolver.resolve(pixel_server_ip, pixel_server_port);

        // Make the connection on the IP address we get from a lookup
        net::connect(ssl_socket.next_layer(), results.begin(), results.end());

        // Perform the SSL handshake
        ssl_socket.handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        http::request<http::empty_body> req{http::verb::get, target, version};
        req.set(http::field::host, pixel_server_ip);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(ssl_socket, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(ssl_socket, buffer, res);

        // Write the message to stdout
        std::cout << res << std::endl;

        // Gracefully close the socket
        beast::error_code ec;
        ssl_socket.shutdown(ec);

        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

    } catch (std::exception const &e) {
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
void send_command(ssl::stream <tcp::socket> &socket, const std::string &cmd) {
    boost::asio::write(socket, boost::asio::buffer(cmd));
}


/*
    Reads one line of SMTP response, bcs each line ends with "\r\n"

    boost::asio::streambuf -> buffer type specifically designed for stream-oriented I/O
    std::istream -> input stream class to handle input streams
*/
std::string read_response(ssl::stream <tcp::socket> &socket) {
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");
    std::istream response_stream(&response);
    std::string response_line;
    std::getline(response_stream, response_line);
    return response_line;
}

/*
    Encodes given string into Base64 format
fix
    Required as Base64 is used in SMTP for encoding credentials

    boost::beast::detail::base64::encoded_size(input.size()) -> calculates the required size of the encoded output based on the input size
    boost::beast::detail::base64::encode -> Boost function that performs the Base64 encoding
*/
std::string base64_encode(const std::string &input) {
    std::string output;
    output.resize(boost::beast::detail::base64::encoded_size(input.size()));
    boost::beast::detail::base64::encode(&output[0], input.data(), input.size());
    return output;
}


void send_email(ssl::stream <tcp::socket> &ssl_socket, const std::string &recipient_email,
                const std::string &subject, const std::string &body) {

    std::string response;

    // MAIL FROM
    std::string mail_from = "MAIL FROM:<" + std::string(EMAIL) + ">\r\n";
    send_command(ssl_socket, mail_from);
    response = read_response(ssl_socket);
    std::cout << "MAIL FROM Response: " << response << std::endl;

    // RCPT TO
    std::string rcpt_to = "RCPT TO:<" + recipient_email + ">\r\n";
    send_command(ssl_socket, rcpt_to);
    response = read_response(ssl_socket);
    std::cout << "RCPT TO Response: " << response << std::endl;

    // DATA
    send_command(ssl_socket, "DATA\r\n");
    response = read_response(ssl_socket);
    std::cout << "DATA Response: " << response << std::endl;

    // Email headers and content
    std::string from = "From: Your Name <" + std::string(EMAIL) + ">\r\n";
    std::string to = "To: <" + recipient_email + ">\r\n";
    std::string content_type = "Content-Type: text/html; charset=UTF-8\r\n";
    std::string mime_version = "MIME-Version: 1.0\r\n";

    // Wrap the body in HTML tags
    std::string html_body = "<html><body>" + body + "</body></html>";

    std::string email_content =
            "Subject: " + subject + "\r\n" + from + to + mime_version + content_type + "\r\n" + body + "\r\n.\r\n";

    // Send the email content
    send_command(ssl_socket, email_content);

    response = read_response(ssl_socket);
    std::cout << "EMAIL Response: " << response << std::endl;
}


void send_emails(const std::string &department_code, const std::string &email_template, const std::string &email_list) {
    try {
        /*
            Section 1: Creating SSL context/sockets, resolving SMTP domains
        */

        // Create SSL context and socket
        // Creating the SSL context that defines the set of parameters we want
        // for using SSL/TLS protocols
        boost::asio::io_context io_context;
        ssl::context ctx(ssl::context::tlsv12_client);
        ssl::stream <tcp::socket> ssl_socket(io_context, ctx);

        // tells SSL context (the ctx) to use system's default locations for trusted root certificate
        ctx.set_default_verify_paths();

        // Resolve SMTP server and connect
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(SMTP_SERVER, "465");

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
        std::cout << "Sending EHLO" << std::endl;
        std::string ehlo_command = "EHLO " + SMTP_DOMAIN + "\r\n";
        send_command(ssl_socket, ehlo_command);

        std::string response = read_response(ssl_socket);
        std::cout << "EHLO Response: " << response << std::endl;

        // TODO: This doesn't work
        while (response[3] == '-') {
            response = read_response(ssl_socket);
            std::cout << "Server: " << response << std::endl;
        }

        // Authentication
        std::cout << "Sending AUTH LOGIN" << std::endl;
        send_command(ssl_socket, "AUTH LOGIN\r\n");
        response = read_response(ssl_socket);
        std::cout << "AUTH response: " << response << std::endl;

        // Send encoded_email
        std::string encoded_email = base64_encode(EMAIL) + "\r\n";
        send_command(ssl_socket, encoded_email);
        response = read_response(ssl_socket);
        std::cout << "AUTH response: " << response << std::endl;

        // Send encoded password
        std::string encoded_password = base64_encode(PASSWORD) + "\r\n";
        send_command(ssl_socket, encoded_password);
        response = read_response(ssl_socket);
        std::cout << "AUTH response: " << response << std::endl;

        // Read the CSV file
        std::vector<Recipient> recipients;
        read_csv(email_list, recipients);

        // Filter recipients based on department code
        std::vector<Recipient> filtered_recipients;
        for (const auto &recipient: recipients) {
            if (department_code == "all" || recipient.department_code == department_code) {
                filtered_recipients.push_back(recipient);
            }
        }

        if (filtered_recipients.empty()) {
            std::cout << "No recipients found for the specified department code.\n";
            return;
        }

        // Read email template
        std::string email_subject;
        std::string email_body;
        read_email_template(email_template, email_subject, email_body);

        // Map to keep track of emails sent per department
        std::map<std::string, int> department_counts;

        // Loop through each recipient and send email
        for (const auto &recipient: filtered_recipients) {
            // Personalize subject and body
            std::string personalized_subject = email_subject;
            std::string personalized_body = email_body;

            replace_placeholders(personalized_subject, recipient);
            replace_placeholders(personalized_body, recipient);

            // Include tracking pixel
            std::string tracking_pixel =
                    "<img src=\"https://" + pixel_server_ip +
                    "/image?" + generate_random_query() + "\" alt=\"image\" style=\"display:none\" />";

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
        for (const auto &entry: department_counts) {
            std::cout << "Department " << entry.first << ": " << entry.second << " emails sent.\n";
        }

        // QUIT command to end SMTP session
        send_command(ssl_socket, "QUIT\r\n");
        std::string response_quit = read_response(ssl_socket);
        std::cout << "Server: " << response_quit << std::endl;

    } catch (std::exception &e) {
        std::cerr << "Exception during send_emails: " << e.what() << std::endl;
    }
}

void handle_command(const std::string &command) {
    if (command == "send_email") {
        std::cout << "Executing email sending function." << std::endl;
        send_emails(DEFAULT_DEPARTMENT_CODE, DEFAULT_EMAIL_TEMPLATE, DEFAULT_MAIL_CSV_PATH);
        std::cout << "Email sending function completed." << std::endl;
    } else {
        std::cout << "Unknown command: " << command << std::endl;
    }
}

void create_txt_file(const std::string &filename, const std::string &content) {
    // Open the file in write mode
    std::ofstream out_file(filename);
    
    if (!out_file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // Write the content to the file
    out_file << content;

    // Close the file
    out_file.close();
    std::cout << "File created successfully: " << filename << std::endl;
}

// Function to create a .csv file and write recipient data
void create_csv_file(const std::string &filename, const std::vector<Recipient> &recipients) {
    // Open the file in write mode
    std::ofstream out_file(filename);

    if (!out_file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // Write the CSV header
    out_file << "Department Code,Email,Name\n";

    // Write each recipient's data
    for (const auto &recipient : recipients) {
        out_file << recipient.department_code << ","
                 << recipient.email << ","
                 << recipient.name << "\n";
    }

    // Close the file
    out_file.close();
    std::cout << "CSV file created successfully: " << filename << std::endl;
}

int main(int argc, char *argv[]) {
    // std::string email_template = DEFAULT_EMAIL_TEMPLATE;
    // std::string email_list = DEFAULT_MAIL_CSV_PATH;

    // if (argc < 2) {
    //     std::cout << "Usage: ./smart_mailer <command> [options]\n";
    //     std::cout << "Commands:\n";
    //     std::cout << "  send [recipient] [--template <email_template.txt>] [--list <email_list.csv>]\n";
    //     std::cout << "  get_count\n";
    //     return 1;
    // }

    // std::string command = argv[1];

    // for (int i = 2; i < argc; ++i) {
    //     if (std::string(argv[i]) == "--template" && i + 1 < argc) {
    //         email_template = argv[++i];
    //     } else if (std::string(argv[i]) == "--list" && i + 1 < argc) {
    //         email_list = argv[++i];
    //     }
    // }

    // if (command == "send") {
    //     std::string recipient = "all"; // Default recipient
    //     if (argc >= 3) {
    //         recipient = argv[2]; // Get recipient from command line
    //     }
    //     send_emails(recipient, email_template, email_list);
    // } else if (command == "get_count") {
    //     get_count();
    // } else {
    //     std::cout << "Invalid command. Please use 'send' or 'get_count'.\n";
    //     return 1;
    // }

    // return 0;

    boost::asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8081)); // Listening on port 8081

    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, "\n");
        std::istream is(&buffer);
        std::string command;
        std::getline(is, command);

        handle_command(command);
    }

    return 0;
}
