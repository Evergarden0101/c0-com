#include "argparse.hpp"
#include "fmt/core.h"

#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"
#include "fmts.hpp"

#include "binary/file.h"
#include "binary/vm.h"
#include "binary/exception.h"
#include "util/print.hpp"

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <exception>

std::vector<C0::Token> _tokenize(std::istream &input) {
    C0::Tokenizer tkz(input);
    auto p = tkz.AllTokens();
    if (p.second.has_value()) {
        fmt::print(stderr, "Tokenization error: {}\n", p.second.value());
        exit(2);
    }
    return p.first;
}

/*
void Tokenize(std::istream &input, std::ostream &output) {
    auto v = _tokenize(input);
    for (auto &it : v)
        output << fmt::format("{}\n", it);
    return;
}
*/

void assemble_text(std::ifstream *in, std::ofstream *out, bool run = false) {
    try {
        File f = File::parse_file_text(*in);
        //f.output_text(std::cout);
        f.output_binary(*out);
        if (run) {
            auto avm = std::move(vm::VM::make_vm(f));
            avm->start();
        }
    }
    catch (const std::exception &e) {
        println(std::cerr, e.what());
    }
}


void Analyse(std::istream &input, std::ostream &output) {
    auto tks = _tokenize(input);
    C0::Analyser analyser(tks);
    auto p = analyser.Analyse();
    if (p.second.has_value()) {
        fmt::print(stderr, "Syntactic analysis error: {}\n", p.second.value());
        exit(2);
    }
    auto v = p.first;
    output << ".constants:\n";
    for (long long unsigned int i = 0; i < v.cons.size(); i++) {
        if (v.cons.at(i).type == 0)
            output << i << " S \"" << v.cons.at(i).s << "\"\n";
        else if (v.cons.at(i).type == 1)
            output << i << " I " << v.cons.at(i).i << "\n";
    }

    output << ".start:\n";
    for (long long unsigned int i = 0; i < v.ins.size(); i++)
        output << i << " " << fmt::format("{}\n", v.ins.at(i));

    output << ".functions:\n";
    for (long long unsigned int i = 0; i < v.func.size(); i++) {
        output << i << " " << v.func.at(i).name_index << " " << v.func.at(i).params_size << " " << v.func.at(i).level
               << "\n";
    }

    for (long long unsigned int i = 0; i < v.func.size(); i++) {
        output << ".F" << i << ":\n";
        for (long long unsigned int j = 0; j < v.func.at(i)._instructions.size(); j++)
            output << j << " " << fmt::format("{}\n", v.func.at(i)._instructions.at(j));
    }
    return;
}



int main(int argc, char **argv) {
    //输出格式 undone
    argparse::ArgumentParser program("C0");
    program.add_argument("input")
            .help("speicify the file to be compiled.");
    program.add_argument("-c")
            .default_value(false)
            .implicit_value(true)
            .help("generate binary file for the input file.");
    program.add_argument("-s")
            .default_value(false)
            .implicit_value(true)
            .help("generate assemble file for the input file.");
    program.add_argument("-o", "--output")
            .required()
            .default_value(std::string("-"))
            .help("specify the output file.");
    //program.add_argument("不提供任何参数时，默认为 -h\n"
    //                     "提供 input 不提供 -o file 时，默认为 -o out");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err) {
        fmt::print(stderr, "{}\n\n", err.what());
        program.print_help();
        exit(2);
    }
    //printf("over\n");

    auto input_file = program.get<std::string>("input");
    auto output_file = program.get<std::string>("--output");
    std::istream *input;
    std::ifstream *ifin;
    std::ostream *output;
    std::ifstream inf;
    std::ofstream outf;
    if (input_file != "-") {
        inf.open(input_file, std::ios::in);
        if (!inf) {
            fmt::print(stderr, "Fail to open {} for reading.\n", input_file);
            exit(2);
        }
        input = &inf;
    } else {
        input = &std::cin;
        //ifin = &std::cin;
    }
    if (output_file != "-") {
            outf.open(output_file, std::ios::out | std::ios::trunc);
        if (!outf) {
            fmt::print(stderr, "Fail to open {} for writing.\n", output_file);
            exit(2);
        }
        output = &outf;
    } else
        output = &std::cout;
    if (program["-s"] == true && program["-c"] == true) {
        fmt::print(stderr, "You can only generate binary or assemble file at one time.");
        exit(2);
    }
    //二进制 undone
    if (program["-c"] == true) {
        outf.close();
        outf.open(input_file + ".s.txt", std::ios::out | std::ios::trunc);
        output = &outf;
        Analyse(*input, *output);
        outf.close();
        inf.close();
        inf.open(input_file + ".s.txt", std::ios::in);
        ifin = &inf;
        outf.open(output_file, std::ios::binary | std::ios::out | std::ios::trunc);
        output = &outf;
        assemble_text(ifin, dynamic_cast<std::ofstream *>(output), false);
    } else if (program["-s"] == true) {
        Analyse(*input, *output);
    }
    else {
        fmt::print(stderr, "You must choose binary or assemble file.");
        exit(2);
    }
    inf.close();
    outf.close();
    return 0;
}