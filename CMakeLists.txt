cmake_minimum_required(VERSION 3.20)
project(Regex)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(Regex
        DFA.cpp
        DFA.h
        Lexer.cpp
        Lexer.h
        main.cpp
        NFA.cpp
        NFA.h
        Pattern.h
        Regex.cpp
        Regex.h
        RegexException.cpp
        RegexException.h
        Token.h)
