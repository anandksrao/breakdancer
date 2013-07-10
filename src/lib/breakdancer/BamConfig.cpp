#include "BamConfig.hpp"

#include "BamIO.hpp"
#include "Options.hpp"
#include "Read.hpp"

#include <boost/regex.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/assign/list_of.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <memory>

using boost::assign::map_list_of;
using boost::container::flat_map;
using namespace std;


std::string translate_legacy_config_token(std::string const& tok) {
    // The point of all the original legacy parsing code was to do something
    // close to the original regular expressions in the perl version of
    // breakdancer. For now, we'll just map hits on those original regexes
    // to standard field names that we define. Ultimately, we want to
    // replace this config file format anyway, so there isn't much use
    // in doing something extremely fancy.
    //
    // Defining a config file format is not really the place to allow this level
    // of flexibility. This code should not be carried forward to any new config
    // format that gets developed.
    using boost::regex;
    static flat_map<regex, string> TOK_MAP = map_list_of
        (regex("map$", regex::icase), "bam_file")
        (regex("mean\\w*$", regex::icase), "insert_size_mean")
        (regex("std\\w*$", regex::icase), "insert_size_stddev")
        (regex("readlen\\w*$", regex::icase), "read_length")
        (regex("upp\\w*$", regex::icase), "insert_size_upper_cutoff")
        (regex("low\\w*$", regex::icase), "insert_size_lower_cutoff")
        (regex("map\\w*qual\\w*$", regex::icase), "min_map_qual")
        (regex("lib\\w*$", regex::icase), "library_name")
        (regex("samp\\w*$", regex::icase), "sample_name")
        ;

    typedef flat_map<regex, string>::const_iterator TIter;
    for (TIter iter = TOK_MAP.begin(); iter != TOK_MAP.end(); ++iter) {
        regex const& re = iter->first;
        boost::smatch match;
        // Note: the original perl version didn't force tokens to begin at
        // any particular point, i.e., they could be prefixed. We will
        // retain that behavior for now
        if (boost::regex_search(tok, match, re))
            return iter->second;
    }

    return string();
}

namespace {
    // augmenting function for reading config file: apply specifically to flag = 0, but the string appeared before, so search the following ones
    string search_more(string line, string search, size_t pos_begin){
        size_t pos = line.find(search, pos_begin);
        string ret;
        if(pos != string::npos){
            size_t pos_begin = line.find(":", pos);
            if(pos_begin != string::npos){
                string substr_tmp = line.substr(pos, pos_begin-pos);
                if(substr_tmp.find("\t") != string::npos || substr_tmp.find(" ") != string::npos){
                    ret = search_more(line, search, pos_begin);
                    return ret;
                }
                else{
                    size_t pos_end = line.find("\t", pos_begin);
                    if(pos_end == string::npos)
                        pos_end = line.find("\0",pos_begin);
                    size_t n = pos_end - pos_begin - 1;
                    return line.substr(pos_begin+1,n);
                }
            }
            else
                return "NA";
        }
        else
            return "NA";
        return "NA";
    }


    string get_from_line(string line,string search,int flag){
        size_t pos;
        if(flag == 1)
            pos = line.find(search + ":");
        else
            pos = line.find(search);
        if(pos != string::npos){
            if(flag == 1){
                pos = pos + search.length() + 1;
                size_t pos_end = line.find("\t",pos);
                if(pos_end == string::npos)
                    pos_end = line.find("\0",pos);
                size_t n = pos_end - pos;
                return line.substr(pos,n);
            }
            else{
                pos = pos + search.length();
                size_t pos_begin = line.find(":", pos);
                if(pos_begin != string::npos){
                    string substr_tmp = line.substr(pos,pos_begin-pos);
                    if(substr_tmp.find("\t") != string::npos || substr_tmp.find(" ") != string::npos){
                        return search_more(line, search, pos_begin);
                    }
                    else{
                        size_t pos_end = line.find("\t",pos_begin);
                        if(pos_end == string::npos)
                            pos_end = line.find("\0", pos_begin);
                        size_t n = pos_end - pos_begin - 1;
                        return line.substr(pos_begin+1,n);
                    }
                }
                else
                    return "NA";
            }
        }
        else
            return "NA";
        return "NA";
    }

    // augmenting fucntion for reading config file
    string get_from_line_two(string line,string search1,string search2,int flag2){
        size_t pos = line.find(search1);
        size_t pos2;
        if(pos != string::npos){
            pos = pos + search1.length();
            pos2 = line.find(search2,pos);
            if(pos2 != string::npos){
                if(flag2 == 1){
                    pos2 = pos2 + search2.length();
                    if(line.substr(pos2,1).compare(":") == 0){
                        size_t pos_end = line.find("\t",pos2);
                        if(pos_end == string::npos)
                            pos_end = line.find("\0",pos2);
                        size_t n = pos_end - pos2 - 1;
                        return line.substr(pos2+1, n);
                    }
                    else
                        return "NA";
                }
                else{
                    pos2 = pos2 + search2.length();
                    size_t pos_begin = line.find(":", pos2);
                    if(pos_begin != string::npos){

                        string substr_tmp = line.substr(pos,pos_begin-pos);
                        if(substr_tmp.find("\t") != string::npos || substr_tmp.find(" ") != string::npos)
                            return "NA";

                        size_t pos_end = line.find("\t",pos2);
                        if(pos_end == string::npos)
                            pos_end = line.find("\0",pos2);
                        size_t n = pos_end - pos_begin - 1;
                        return line.substr(pos_begin, n);
                    }
                    else
                        return "NA";
                }
            }
            else
                return "NA";
        }
        else
            return "NA";
        return "NA";
    }
}


BamConfig::BamConfig()
    : max_read_window_size(1e8)
    , max_readlen(0)
{
}

BamConfig::BamConfig(std::istream& in, Options const& opts)
    : max_read_window_size(1e8)
    , max_readlen(0)
{
    map<string, LibraryConfig> temp_lib_config;
    string line;
    while(getline(in, line)) {
        if(line.empty())
            break;

        // analyze the line
        string fmap = get_from_line(line,"map",1);
        string mean_ = get_from_line(line,"mean",0);
        string std_ = get_from_line(line,"std",0);
        string readlen_ = get_from_line(line,"readlen",0);
        string upper_ = get_from_line(line,"upp",0);
        string lower_ = get_from_line(line,"low",0);
        string mqual_ = get_from_line_two(line,"map","qual",0);
        string lib = get_from_line(line,"lib",0);
        float mean=0.0f,std=0.0f,readlen=0.0f,upper=0.0f,lower=0.0f;
        int mqual;
        if(lib.compare("NA")==0)
            lib = get_from_line(line,"samp",0);


        string readgroup = get_from_line(line,"group",1);
        if(readgroup.compare("NA")==0)
            readgroup = lib;
        _readgroup_library[readgroup] = lib;

        readgroup_platform[readgroup] = get_from_line(line,"platform",1);

        string exe = get_from_line(line,"exe",0);
        if(!opts.prefix_fastq.empty()) {
            //ofstream ReadsOut[lib.append("1")](prefix_fastq.append(lib).append(".1.fastq"), ios::out | ios::app | ios::binary);
            ReadsOut[lib + "1"] = opts.prefix_fastq + "." + lib + ".1.fastq";
            ofstream ReadsOutTmp;
            ReadsOutTmp.open(ReadsOut[lib+"1"].c_str());
            if(!ReadsOutTmp.is_open())
                cout << "unable to open " << opts.prefix_fastq << "." << lib << ".1.fastq, check write permission\n";
            ReadsOutTmp.close();
            ReadsOut[lib + "2"] = opts.prefix_fastq + "." + lib + ".2.fastq";
            ReadsOutTmp.open(ReadsOut[lib+"2"].c_str());
            if(!ReadsOutTmp.is_open())
                cout << "unable to open " << opts.prefix_fastq << "." << lib << ".2.fastq, check write permission\n";
            ReadsOutTmp.close();
        }

        LibraryConfig lib_config;
        lib_config.name = lib;
        lib_config.bam_file = fmap;

        if(mqual_.compare("NA")){
            mqual = atoi(mqual_.c_str());
            lib_config.min_mapping_quality = mqual;
        } else {
            lib_config.min_mapping_quality = -1;
        }
        _bam_library[fmap] = lib;

        if(mean_ != "NA" && std_ != "NA") {
            mean = atof(mean_.c_str());
            std = atof(std_.c_str());
            if(!upper_.compare("NA") || !lower_.compare("NA")){
                upper = mean + std*float(opts.cut_sd);
                lower = mean - std*float(opts.cut_sd);
                lower = lower > 0 ? lower : 0;//atof(lower_.c_str()) > 0 ? atof(lower_.c_str()):0; this is not related with lower_
            }
            else{
                upper = atof(upper_.c_str());
                lower = atof(lower_.c_str());
            }
        }

        if(readlen_.compare("NA")){
            readlen = atof(readlen_.c_str());
        }
        max_readlen = max_readlen < readlen ? readlen:max_readlen;

        lib_config.mean_insertsize = mean;
        lib_config.std_insertsize = std;
        lib_config.uppercutoff = upper;
        lib_config.lowercutoff = lower;
        lib_config.readlens = readlen;

        // This is silly, library info can be repeated in the config file
        // hopefully whatever we are clobbering is equivalent!
        temp_lib_config[lib] = lib_config;

        if(exes.find(fmap) == exes.end())
            exes[fmap] = exe.compare("NA")?exe:"cat";
        else if(exes[fmap].compare(exe) != 0){
            throw runtime_error(
                "Please use identical exe commands to open the same input file.\n"
                );
        }

        int tmp = mean - readlen*2;    // this determines the mean of the max of the SV flanking region
        max_read_window_size = std::min(max_read_window_size, tmp);
    }


    for (map<string, LibraryConfig>::iterator i = temp_lib_config.begin(); i != temp_lib_config.end(); ++i) {
        i->second.index = _library_config.size();
        _lib_names_to_indices[i->first] = i->second.index;
        _library_config.push_back(i->second);
    }


    typedef ConfigMap<string, string>::type::const_iterator IterType;
    for (IterType iter = _bam_library.begin(); iter != _bam_library.end(); ++iter) {
        _bam_files.push_back(iter->first);
    }

    max_read_window_size = std::max(max_read_window_size, 50);
}

