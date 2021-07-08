/*!
@file
@brief Definition of the VCDFileParser class
*/

/*
    Modified by Donn on 2021-07-07:
        * Fixed memory leak
        * Made it possible to parse a string instead of a file
        * Added JSON Serialization Options
*/

#include "VCDFileParser.hpp"
#include <sstream>
#include <cstring>
#include <system_error>
#include <iomanip>
#include <functional>

VCDFileParser::VCDFileParser() {


    this -> start_time = -std::numeric_limits<decltype(start_time)>::max();
    this -> end_time = std::numeric_limits<decltype(end_time)>::max() ;

    this->trace_scanning = false;
    this->trace_parsing = false;
}

VCDFileParser::~VCDFileParser()
{
    if (parse_string) {
        free(parse_string);
    }
    while (!scopes.empty()) {
        delete scopes.top();
        scopes.pop();
    }
}

VCDFile *VCDFileParser::parse_file(const std::string & filepath, const std::string & parse_string)
{
    this->filepath = filepath;
    if (parse_string.length() != 0) {
        this->parse_string = strdup(parse_string.c_str());
    } else {
        this->parse_string = NULL;
    }

    scan_begin();

    this->fh = new VCDFile();
    VCDFile *tr = this->fh;

    this->fh->root_scope = new VCDScope;
    this->fh->root_scope->name = std::string("$root");
    this->fh->root_scope->type = VCD_SCOPE_ROOT;

    this->scopes.push(this->fh->root_scope);

    this -> fh -> root_scope = new VCDScope;
    this -> fh -> root_scope -> name = std::string("");
    this -> fh -> root_scope -> type = VCD_SCOPE_ROOT;
    
    this -> scopes.push(this -> fh -> root_scope);
    
    tr -> add_scope(scopes.top());

    VCDParser::parser parser(*this);

    parser.set_debug_level(trace_parsing);

    int result = parser.parse();

    this->scopes.pop();

    scan_end();

    if (result == 0)
    {
        this->fh = nullptr;
        return tr;
    }
    else
    {
        tr = nullptr;
        delete this->fh;
        return nullptr;
    }
}

void VCDFileParser::error(const VCDParser::location &l, const std::string &m)
{
    std::cerr << "line " << l.begin.line
              << std::endl;
    std::cerr << " : " << m << std::endl;
}

void VCDFileParser::error(const std::string & m){
    std::cerr << " : "<<m<<std::endl;
}

std::string VCDFileParser::empty = "";

std::string represent_timescale(VCDTimeUnit u) {
    if (u == TIME_MS) {
        return "ms";
    } else if (u == TIME_US) {
        return "us";
    } else if (u == TIME_NS) {
        return "ns";
    } else if (u == TIME_PS) {
        return "ps";
    } else {
        return "s";
    }
}

std::string represent_VCDVarType(VCDVarType t) {
    if (t == VCD_VAR_EVENT) {
        return "event";
    } else if (t == VCD_VAR_INTEGER) {
        return "integer";
    } else if (t == VCD_VAR_PARAMETER) {
        return "parameter";
    } else if (t == VCD_VAR_REAL) {
        return "real";
    } else if (t == VCD_VAR_REALTIME) {
        return "realtime";
    } else if (t == VCD_VAR_REG) {
        return "reg";
    } else if (t == VCD_VAR_SUPPLY0) {
        return "supply0";
    } else if (t == VCD_VAR_SUPPLY1) {
        return "supply1";
    } else if (t == VCD_VAR_TIME) {
        return "time";
    } else if (t == VCD_VAR_TRI) {
        return "tri";
    } else if (t == VCD_VAR_TRIAND) {
        return "triand";
    } else if (t == VCD_VAR_TRIOR) {
        return "trior";
    } else if (t == VCD_VAR_TRIREG) {
        return "trireg";
    } else if (t == VCD_VAR_TRI0) {
        return "tri0";
    } else if (t == VCD_VAR_TRI1) {
        return "tri1";
    } else if (t == VCD_VAR_WAND) {
        return "wand";
    } else if (t == VCD_VAR_WIRE) {
        return "wire";
    } else if (t == VCD_VAR_WOR) {
        return "wor";
    } else {
        return "unknown";
    }
}

static std::string represent_value(VCDValue* value) {
    std::stringstream output;
    auto type = value->get_type();
    if (type == VCD_SCALAR) {
        output << VCDValue::bit_to_char(value->get_value_bit());
    } else if (type == VCD_VECTOR) {
        auto& bitvector = *value->get_value_vector();
        for(auto& bit: bitvector) {
            output << VCDValue::bit_to_char(bit);
        }
    } else if (type == VCD_REAL) {
        output << value->get_value_real();
    }

    return output.str();
}

static std::string trim(std::string input) {
    auto c = input.c_str();

    auto i = 0;
    while (c[i] == ' ' || c[i] == '\t' || c[i] == '\n') {
        i++;
    }
    auto j = input.length() - 1;
    while (c[j] == ' ' || c[j] == '\t' || c[j] == '\n') {
        j--;
    }

    return std::string(c + i, j - i + 1);
}

static std::string escape(std::string input) {
    std::stringstream output;

    for (auto& c: input) {
        if ('A' <= c && c <= 'Z' || 'a' <= c && c <= 'z' || '0' <= c && c <= '9' || c == ' ' || c == '_') {
            output << c;
        } else {
            output << "\\u" << std::setw(4) << std::setfill('0') << std::hex << int(c);
        }
    }

    return output.str();
}

char* VCDToJSON(const char* input_vcd_c) {
    auto input_vcd = std::string(input_vcd_c);
    VCDFileParser parser;
    VCDFile *file = parser.parse_file("*", input_vcd);

    std::stringstream outstream(std::ios_base::in | std::ios_base::out);

    VCDTime endtime = -1.0;
    for (auto &t: *file->get_timestamps()) {
        if (t > endtime) {
            endtime = t;
        }
    }

    auto &scopes = *file->get_scopes();

    outstream << "{" << '\n';;
        outstream << "\"date\": " << '"' << escape(trim(file->date)) << '"' << ',' << '\n';
        outstream << "\"version\": " << '"' << escape(trim(file->version)) << '"' << ',' << '\n';
        outstream << "\"comment\": " << '"' << escape(trim(file->comment)) << '"' << ',' << '\n';
        outstream << "\"timescale\": " << '"' << file->time_resolution << represent_timescale(file->time_units) << '"' << ',' << '\n';
        outstream << "\"endtime\": " << endtime << ',' << '\n';
        outstream << "\"data\": " << '{' << '\n';
        std::function<void(VCDScope*)> handleScopes = [&](VCDScope* currentScope) {
                outstream << "\"signals\": " << '[' << '\n';
                    for (auto i = 0; i < currentScope->signals.size(); i += 1) {
                        auto& signal = *currentScope->signals[i];
                        auto& values = *file->get_signal_values(signal.hash);

                        if (i) {
                            outstream << ',';
                        }
                        outstream << '{' << '\n';
                            outstream << "\"name\": " << '"' << escape(signal.reference) << '"' << ',' << '\n'; 
                            outstream << "\"hash\": " << '"' << escape(signal.hash) << '"' << ',' << '\n'; 
                            outstream << "\"type\": " << '"' << represent_VCDVarType(signal.type) << '"' << ',' << '\n';
                            outstream << "\"size\": " << signal.size << ',' << '\n';
                            if (signal.rindex != -1) {
                            outstream << "\"left_index\": " << signal.lindex << ',' << '\n';
                            outstream << "\"right_index\": " << signal.rindex << ',' << '\n';
                            }
                            outstream << "\"waveform\": " << '[' << '\n';
                            for (auto j = 0; j < values.size(); j += 1) {
                                auto& value = *values[j];
                                if (j) {
                                    outstream << ',';
                                }
                                outstream << '{' << '\n';
                                outstream << "\"time\": " << '"' << value.time << '"' << ',' << '\n'; 
                                outstream << "\"value\": " << '"' << represent_value(value.value) << '"' << '\n';
                                outstream << '}';


                            }
                            outstream << ']' << '\n';

                        outstream << '}';
                    }
                outstream << ']' << ',';
                outstream << "\"subscopes\": " << '{' << '\n';
                    for (auto i = 0; i < currentScope->children.size(); i += 1) {
                        auto& scope = *currentScope->children[i];
                        if (i) {
                            outstream << ',';
                        }
                        outstream << '"' << escape(scope.name) << '"' << ':' << '{' << '\n';
                        handleScopes(&scope);
                        outstream << '}' << '\n';
                    }
                outstream << '}' << '\n';
        };
        handleScopes(file->root_scope);
        outstream << '}' << '\n';
    outstream << '}' << '\n';

    delete file;
    return strdup(outstream.str().c_str());
}