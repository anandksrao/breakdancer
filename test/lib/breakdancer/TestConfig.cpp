#include "breakdancer/Options.hpp"
#include "breakdancer/BDConfig.hpp"
#include "breakdancer/BamConfig.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <set>

using boost::assign::map_list_of;
using namespace std;

namespace {
    // FIXME: figure out how to generate these or something...
    string _cfg_str =
        "readgroup:rg1	platform:illumina	map:x.bam	readlen:90.00	lib:lib1	num:10001	lower:277.03	upper:525.50	mean:467.59	std:31.91	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg2	platform:illumina	map:x.bam	readlen:90.00	lib:lib1	num:10001	lower:277.03	upper:525.50	mean:467.59	std:31.91	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg3	platform:illumina	map:x.bam	readlen:90.00	lib:lib1	num:10001	lower:277.03	upper:525.50	mean:467.59	std:31.91	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg4	platform:illumina	map:x.bam	readlen:90.00	lib:lib1	num:10001	lower:277.03	upper:525.50	mean:467.59	std:31.91	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg5	platform:illumina	map:x.bam	readlen:90.00	lib:lib1	num:10001	lower:277.03	upper:525.50	mean:467.59	std:31.91	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg6	platform:illumina	map:x.bam	readlen:90.00	lib:lib1	num:10001	lower:277.03	upper:525.50	mean:467.59	std:31.91	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg7	platform:illumina	map:x.bam	readlen:90.00	lib:lib1	num:10001	lower:277.03	upper:525.50	mean:467.59	std:31.91	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg8	platform:illumina	map:y.bam	readlen:90.00	lib:lib2	num:10001	lower:311.36	upper:532.53	mean:475.76	std:28.67	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg9	platform:illumina	map:y.bam	readlen:90.00	lib:lib2	num:10001	lower:311.36	upper:532.53	mean:475.76	std:28.67	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg10	platform:illumina	map:y.bam	readlen:90.00	lib:lib2	num:10001	lower:311.36	upper:532.53	mean:475.76	std:28.67	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg11	platform:illumina	map:y.bam	readlen:90.00	lib:lib2	num:10001	lower:311.36	upper:532.53	mean:475.76	std:28.67	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg12	platform:illumina	map:y.bam	readlen:90.00	lib:lib2	num:10001	lower:311.36	upper:532.53	mean:475.76	std:28.67	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg13	platform:illumina	map:y.bam	readlen:90.00	lib:lib2	num:10001	lower:311.36	upper:532.53	mean:475.76	std:28.67	SWnormality:minus infinity	exe:samtools view\n"
        "readgroup:rg14	platform:illumina	map:y.bam	readlen:90.00	lib:lib2	num:10001	lower:311.36	upper:532.53	mean:475.76	std:28.67	SWnormality:minus infinity	exe:samtools view\n"
    ;
}

class TestConfig : public ::testing::Test {
public:
    void SetUp() {
        _cfg_stream << _cfg_str;
    }

protected:
    std::stringstream _cfg_stream;
    Options _opts;
};

class TestableBDConfig : public BDConfig {
public:
    TestableBDConfig(istream& in)
        : BDConfig(in)
    {
    }

    using BDConfig::_entries;
};

TEST_F(TestConfig, translate_legacy_config_token) {
    map<string, string> expected_translations = map_list_of
        ("map", "bam_file")

        ("mean", "insert_size_mean")
        ("mean_insert", "insert_size_mean")
        ("mean_insert_size", "insert_size_mean")

        ("std", "insert_size_stddev")
        ("stddev", "insert_size_stddev")
        ("insert_stddev", "insert_size_stddev")
        ("insert_size_stddev", "insert_size_stddev")
        ("stddev_insert", "insert_size_stddev")
        ("stddev_insert_size", "insert_size_stddev")

        ("readlen", "read_length")
        ("rEaDlEnGtH", "read_length")
        ("average_readlen", "read_length")
        ("average_readlength", "read_length")

        ("upp", "insert_size_upper_cutoff")
        ("upper", "insert_size_upper_cutoff")
        ("uppEr_cutOff", "insert_size_upper_cutoff")
        ("inseRt_size_uPper_cutoff", "insert_size_upper_cutoff")

        ("low", "insert_size_lower_cutoff")
        ("lower", "insert_size_lower_cutoff")
        ("lower_cuToff", "insert_size_lower_cutoff")
        ("insert_size_lower_cutoff", "insert_size_lower_cutoff")

        ("mapqual", "min_map_qual")
        ("mapPing_quAlity", "min_map_qual")

        ("lib", "library_name")
        ("libname", "library_name")
        ("library_name", "library_name")

        ("samp", "sample_name")
        ("sample", "sample_name")
        ("samplename", "sample_name")
        ("sample_name", "sample_name")
        ;

    EXPECT_EQ("", translate_legacy_config_token("ZIOJFksfjlaiaowinfd"));

    typedef map<string, string>::const_iterator TIter;
    for (TIter i = expected_translations.begin(); i != expected_translations.end(); ++i) {
        string src = i->first;
        string const& expected = i->second;
        EXPECT_EQ(expected, translate_legacy_config_token(src));
        boost::to_upper(src);
        EXPECT_EQ(i->second, translate_legacy_config_token(src));
        boost::to_lower(src);
        EXPECT_EQ(i->second, translate_legacy_config_token(src));
    }
}

// FIXME: let's not be tightly coupled to bam files existing on disk!
TEST_F(TestConfig, legacyParse) {
    BamConfig cfg(_cfg_stream, _opts);

    // test "fmaps", mapping input files -> first library they contain?
    ASSERT_EQ(2u, cfg.bam_files().size());
    EXPECT_EQ("x.bam", cfg.bam_files()[0]);
    EXPECT_EQ("y.bam", cfg.bam_files()[1]);

    for (size_t i = 1; i <= 7; ++i) {
        stringstream rg1;
        stringstream rg2;
        rg1 << "rg" << i;
        rg2 << "rg" << i + 7;
        EXPECT_EQ("lib1", cfg.readgroup_library(rg1.str()));
        EXPECT_EQ("lib2", cfg.readgroup_library(rg2.str()));
    }

    // test "exes", the executables perl would use to view the input file?
    ASSERT_EQ(2u, cfg.exes.size());
    EXPECT_EQ(1u, cfg.exes.count("x.bam"));
    EXPECT_EQ(1u, cfg.exes.count("y.bam"));
    EXPECT_EQ("samtools view", cfg.exes.find("x.bam")->second);
    EXPECT_EQ("samtools view", cfg.exes.find("y.bam")->second);

    // test libmaps, mapping library names -> input files?
    ASSERT_EQ(2u, cfg.num_libs());
    EXPECT_EQ(0u, cfg.library_config_by_name("lib1").index);
    EXPECT_EQ(1u, cfg.library_config_by_name("lib2").index);
    EXPECT_THROW(cfg.library_config_by_name("lib3"), out_of_range);

    EXPECT_EQ("lib1", cfg.library_config_by_index(0).name);
    EXPECT_EQ("lib2", cfg.library_config_by_index(1).name);
    EXPECT_THROW(cfg.library_config_by_index(2), out_of_range);
}

TEST_F(TestConfig, bdConfigRawParse) {
    TestableBDConfig cfg(_cfg_stream);
    ASSERT_EQ(14u, cfg._entries.size());

    set<string> expected_readgroups;
    for (size_t i = 0; i < 14; ++i) {
        stringstream rgname;
        rgname << "rg" << i+1;
        expected_readgroups.insert(rgname.str());
        ASSERT_EQ(rgname.str(), cfg._entries[i].readgroup);
    }

    ASSERT_TRUE(expected_readgroups == cfg.readgroups())
        << "Parsed read groups are not as expected";
}
