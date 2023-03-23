#include <Link.hpp>
#include <iostream>
#include <fstream>
#include <map>

std::map<std::string, int> ips;

std::vector<std::string> split(std::string data, std::string delimiter) {
  std::vector<std::string> result;
  size_t pos = 0;
  std::string token;
  while ((pos = data.find(delimiter)) != std::string::npos) {
    token = data.substr(0, pos);
    if (token != "") result.push_back(token);
    data.erase(0, pos + delimiter.length());
  }
  if (data != "") result.push_back(data);
  return result;
}

#include <mailio/message.hpp>
#include <mailio/smtp.hpp>

int sendMessage(std::string email, std::string message) {

	std::ifstream file("smtp");
	std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	std::string pass;
	for (std::string line: split(str, "\n")) {
		if (line.find("KEY=") != std::string::npos) {
			pass = line.substr(4);
		}
	}

	try {
		mailio::message msg;
		msg.from(mailio::mail_address("Levi Hicks", "me@levihicks.dev"));
		msg.add_recipient(mailio::mail_address("", email));
		msg.add_recipient(mailio::mail_address("Levi Hicks", "me@levihicks.dev"));
		msg.subject("Contact Form");
		msg.content("I have received your message. I will get back to you as soon as possible. \r\n\r\n" + message);

		mailio::smtps conn("smtp.mail.me.com", 587);
		conn.authenticate("levicowan2005@icloud.com", pass, mailio::smtps::auth_method_t::START_TLS);
		conn.submit(msg);
		std::cout << "Message sent" << std::endl;
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		return 0;
	}
	return 1;

		
}

std::string replace(std::string data, std::string delimiter, std::string replacement) {
  size_t pos = 0;
  while ((pos = data.find(delimiter)) != std::string::npos) {
    data.replace(pos, delimiter.length(), replacement);
  }
  return data;
}

void Email(Link::Request* req, Link::Response* res) {
	res->SetHeader("Access-Control-Allow-Origin", "*");
	if (ips[req->GetIP()] >= 5) {
		res->SetStatus(429)->SetBody("429 Too Many Requests");
		return;
	}
	ips[req->GetIP()]++;
	std::cout << ips[req->GetIP()] << std::endl;
	std::string email, message;
	std::string body = req->GetBody();
	std::string parameter;
	std::map<std::string, std::string> params;
    std::vector<std::string> arr;
    size_t pos = 0;
    std::string token;
    while ((pos = body.find("&")) != std::string::npos) {
      token = body.substr(0, pos);
      if (token != "") arr.push_back(token);
      body.erase(0, pos + 1);
    }
    if (body != "") arr.push_back(body);
    for (std::string parameter : arr) {
      std::string key = parameter.substr(0, parameter.find_first_of("="));
      std::string value = parameter.substr(parameter.find_first_of("=")+1);
	  params[key] = value;
    }
	email = params["email"];
	message = params["message"];

	if (email == "" || message == "" || sendMessage(email, replace(message, "\\", "\\\\"))==0) {
		res->SetStatus(400)->SetBody("400 Bad Request");
		return;
	}

	res->SetStatus(200)->SetBody("OK");
}

int main() {
	Link::Server server(8080);

  	// 404 Page
  	server.Error(404, [](Link::Request* req, Link::Response* res) {
  		res->SetStatus(404)->SetBody("404 Not Found");
	});
	
	server.Post("/api/email", Email);

	server.Route("OPTIONS", "/api/email", [](Link::Request* req, Link::Response* res) {
		if (req->GetHeader("Access-Control-Request-Method").substr(0, req->GetHeader("Access-Control-Request-Method").length()-1)!="POST") {
			res->SetStatus(405)->SetBody("405 Method Not Allowed");
			return;
		}
		Email(req, res);
	});

	server.EnableSSL("cert.pem", "key.pem");

	server.EnableMultiThreading();

	server.SetStartMessage("Server started on port " + std::to_string(server.GetPort()));
  	server.Start();
	std::cout << server.Status << std::endl;
}
