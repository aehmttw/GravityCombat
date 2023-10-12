#include "Connection.hpp"
#include <iostream>
#include <random>

int main(int argc, char **argv) {
    char mode;
    std::cin >> mode;

    Client client("15466.courses.cs.cmu.edu", "15466"); //connect to a local server at port 1337

	{ //send handshake message:
		Connection &con = client.connection;
		con.send_buffer.emplace_back('H');
		con.send_buffer.emplace_back( 13 );
		con.send_buffer.emplace_back(mode);
		for (char c : std::string("drasan")) {
			con.send_buffer.emplace_back(c);
		}
		for (char c : std::string("mateib")) {
			con.send_buffer.emplace_back(c);
		}

	}

	while (true) {
		client.poll([](Connection *connection, Connection::Event evt){
		},
		0.0 //timeout (in seconds)
		);

		Connection &con = client.connection;

		while (true) {
            if (con.recv_buffer.size() < 2) break;
			char type = char(con.recv_buffer[0]);
			size_t length = size_t(con.recv_buffer[1]);
			if (con.recv_buffer.size() < 2 + length) break;

			if (type == 'T') {
				const char *as_chars = reinterpret_cast< const char * >(con.recv_buffer.data());
				std::string message(as_chars + 2, as_chars + 2 + length);

				std::cout << "T: '" << message << "'" << std::endl;
			} else if (type == 'V') {
                std::cout << "got view\n";
                const char *chars = reinterpret_cast< const char * >(con.recv_buffer.data());

                std::cout << chars[2] << chars[3] << chars[4] << "\n";
                std::cout << chars[5] << chars[6] << chars[7] << "\n";
                std::cout << chars[8] << chars[9] << chars[10] << "\n";

                con.send_buffer.emplace_back('M');
                con.send_buffer.emplace_back(1);

                char input;
                std::cin >> input;

                if (input == 's')
                    con.send_buffer.emplace_back('S');
                else if (input == 'a')
                    con.send_buffer.emplace_back('W');
                else if (input == 'w')
                    con.send_buffer.emplace_back('N');
                else if (input == 'd')
                    con.send_buffer.emplace_back('E');
                else
                    con.send_buffer.emplace_back(input);

            } else {
				std::cout << "Ignored a " << type << " message of length " << length << "." << std::endl;
			}

			con.recv_buffer.erase(con.recv_buffer.begin(), con.recv_buffer.begin() + 2 + length);
		    std::cout << "end poll\n";
        }
	}
}
