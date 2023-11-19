#include <iostream>
#include <set>
#include <functional>
#include <mutex>
#include <signal.h>

#define ASIO_STANDALONE
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"

#include "nlohmann/json.hpp"

using std::string;
using std::mutex;
using std::lock_guard;
using std::thread;

using nlohmann::json;
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::server<websocketpp::config::asio> server;
// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;
typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;

static server s;
static con_list connections;
static mutex connections_lock;
static std::thread *t = nullptr;
static bool running = false;

static void signal_handle(int sig)
{
	if (sig != SIGINT && sig != SIGTERM)
		return;
	std::cout << "get signal " << sig << std::endl;
	running = false;
	if (t != nullptr) {
		t->join();
		delete t;
		t = nullptr;
	}
	std::cout << "websocket server stop..." << std::endl;
	s.stop();
	while (!s.stopped());
	std::cout << "websocket server stoped" << std::endl;
}

void on_open(connection_hdl hdl)
{
	std::cout << "a connection open" << std::endl;
	lock_guard<mutex> lock(connections_lock);
	connections.insert(hdl);
}

void on_close(connection_hdl hdl)
{
	std::cout << "a connection close" << std::endl;
	lock_guard<mutex> lock(connections_lock);
	connections.erase(hdl);
}

// Define a callback to handle incoming messages
void on_message(connection_hdl hdl, message_ptr msg) {
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;
}

void period_task()
{
	while (running) {
		{
			std::cout << "++++++++++++++++" << std::endl;
			lock_guard<mutex> guard(connections_lock);
			for (auto it : connections) {
				connection_hdl hdl = it;
				server::connection_ptr con = s.get_con_from_hdl(hdl);
				if (con != nullptr && con->get_state() == websocketpp::session::state::open)
					s.send(hdl, "it's a message from server", websocketpp::frame::opcode::text);
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

int main()
{
	signal(SIGINT, signal_handle);
	signal(SIGTERM, signal_handle);

	// Create a thread to push message to all clients
	t = new std::thread(period_task);

	running = true;

	// Initialize Asio
	s.init_asio();

	// Set logging settings
	//s.set_error_channels(websocketpp::log::elevel::all);
	s.set_error_channels(websocketpp::log::elevel::none);
	//s.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);
	s.set_access_channels(websocketpp::log::alevel::none);

	// Register handler
	s.set_open_handler(bind(on_open, ::_1));
	s.set_close_handler(bind(on_close, ::_1));
	s.set_message_handler(bind(on_message, ::_1, ::_2));

    // Listen on port 9002
	//s.listen(9002);
	s.listen(websocketpp::lib::asio::ip::tcp::v4(), 9002);

    // Queues a connection accept operation
    s.start_accept();

    // Start the Asio io_service run loop
	std::cout << "websocket server start..." << std::endl;
    s.run();
	std::cout << "websocket server finish" << std::endl;

    return 0;
}