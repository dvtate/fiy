//
// Created by tate on 6/25/25.
//

// This file should only be included by Server.cpp

#pragma once

#include <memory>

#include <boost/beast.hpp>

class Connection;
class Session;

void route_request(std::shared_ptr<Session> conn);

void handle_request(std::shared_ptr<Session> session);