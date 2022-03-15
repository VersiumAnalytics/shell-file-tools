// Copyright 2014 Versium Analytics, Inc.
// License : BSD 3 clause

/**
 * This is to replace the C-version get-filestats.cpp, which is problematic and has some bugs
 * command line is like:
 *  argv[0] [-d delim] [-t] [-1 | -h HEADER-FILE.txt] [-s SAMPLE-SIZE] [-n SAMPLE-COLUMNS-NUM]
 * This project shows the standard way to process command line options
 *
 * For the random sets to be output, it's fixed length, so looks like we should use
 * array instead of vector. But note, since we don't have insertion/deletion operations
 * on the container, so actually we can instead use vector, the complexity will be same for
 * set/get at random location. And we also keep the flexibility for sample shortage case.
 * So we skipped a lot of new(nothrow) usage, which make it robust.
 *
 * Since the file can be very big, we turn to use int64 instead of just int.
 *
 * A typical analyzing of a file with header (csv format) will be like:
 * cat file > argv[0] -1 -d $',' -s 5 -n 5
 * or with standalone header file header.txt, it will be (header.txt and file are both tsv format)
 * cat file > argv[0] -h header.txt
 * Later I default the "-1" option, so if "-h" is omitted, then "-1" is default, meaning the input file has header
 *
 * We should use chrono method to get the elapsed time, but it's not supported on server yet.
 *
 * Notes: since we haven't really done the csv2tsv job, so if input if csv format, there
 * maybe some disturbance, we prefer tsv input. The sample output is fixed to be tsv.
 *
 * January 2021, Sam Fu
 * for performance, reuse string, get rid of silly command line options [-1] [-t], and add one case
 * of no sampling to improve performance.
 * add [-T] [-t] options for trimming, [-e] for escape character choice for csv format [-d $',']
 */

#include <cstdio>
#include <iostream>
#include <cerrno>
#include <cstdint>
#include <vector>
#include <typeinfo>
#include <cstring>
#include <cmath>
#include <cstdarg>
#include <exception>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <list>
#include <utility>
#include <tuple>
#include <array>
#include <deque>
#include <queue>
#include <stack>
#include <set>
#include <unordered_set>
#include <map>
#include <iterator>
#include <forward_list>
#include <algorithm>
#include <locale>
#include <functional>
#include <typeinfo>
#include <random>
#include <memory>
#include <regex>
#include <ctime>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

using namespace std;

constexpr const char TAB = '\t';
constexpr const char COMMA = ',';
constexpr const char Quote = '"';

const int DefaultSampleRecs = 10;
const int DefaultSampleCols = 5;

int verbose_flag = 0;
char delim = '\t';
bool needTrim = true;   // can be reset from command line option -T
char EscapeChar = '"';  // can be set from command line option -e, for csv input
string headerFile;
// bool shortOutput = false;
bool inFileHeader = true;
int numSampleRec = DefaultSampleRecs;
int numSampleCol = DefaultSampleCols;

bool needSampleRow;
bool needSampleCol;

// file size, line size and line count
int64_t fsize = 0;  // real file size, so including all CR
int64_t lmax = 0;   // line (record) maximal length, counting ending CR
int64_t lmin = INT64_MAX;
int64_t lcnt = 0;   // record number

// field count per record, and total fields
int64_t minfpl = INT64_MAX;     // minimal field number per line (record)
int64_t maxfpl = 0;
int64_t fieldtotal = 0;

// property related to each field, like field length
vector <int64_t> minfl;     // one field's minimal length, cross all records
vector <int64_t> maxfl;     // one field's maximal length, cross all records
vector <int64_t> fieldcount;        // it should all be lcnt, but not necessarily for jagged record
vector <int64_t> fieldfillcount;     // a field can be empty, then it's not counted
vector <int64_t> fieldtotalbytes;   // old code has both ByteCtr and TTLFldLen, with later one not counting "0", "0.0".

// sampling for records and fields
vector <string> sampleRecs;
vector <vector<string>> sampleFields;

const char *usage = R"END(
  Usage: %s [-d delim] [-t | -T] [-e escape char] [-h HEADER-FILE.txt] [-s SAMPLE-SIZE] [-n SAMPLE-COLUMNS-NUM]
  -d -- delimiter, if omitted, default to be tabular
  -e -- escape character for csv, default to be double quote
  -t -- tells the tool to trim fields.
  -T -- tells the tool to not trim fields.
  -h -- read headerfile (tab separated or newline separated) and inject field names into output. Omitting this means first line is header.
  -s -- include sample records at bottom of report.
  -n -- include column samples on right of report.

)END";

/**
 * trim the starting and ending blanks
 * Note trimming starting blanks, if any, the complexity is O(n)
 * trimming ending blanks, if any, the complexity is just O(1)
 * @param s input string, trim in place
 */
void trim_ends(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), [](int ch) { return !isspace(ch); }).base(), s.end());
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) { return !isspace(ch); }));
}

/**
 * explode a string into vector<string> using delim as delimter
 * note for each field we have cleaned it using trim_ends
 * @param src string for input
 * @param delim char as delimiter
 * @return vector<string> as output fields
 */
vector <string> explode(string src, char delim) {
    vector <string> fields;
    src.push_back(delim);    // be aware of the gotcha
    stringstream data(src);
    string field;
    while (getline(data, field, delim)) {
        trim_ends(field);
        fields.push_back(field);
    }
    return fields;
}

/**
 * implode a vector<string> into a string using pDelim as delimiter
 * @param fields vector<string> as the input vector
 * @param pDelim const char* as the C-string delimiter
 * @return string as the result
 */
string implode(vector <string> &fields, const char *pDelim) {
    ostringstream oss;
    copy(fields.begin(), fields.end(), ostream_iterator<string>(oss, pDelim));
    string result = oss.str();
    if (!result.empty()) {
        for (int i = 0; i < strlen(pDelim); i++) result.pop_back();
    }
    return result;
}

/**
 * formatted output of DateTime, you can use like "%Y-%m-%d %H:%M:%S", etc.
 * note this is not thread safe. For thread safe, we should use localtime_s, but nobody
 * will care about that less than 1 second difference anyway.
 * @param fmt const char * for the format
 * @return string of the current date time representation
 */
string formattedDT(const char *fmt) {
    auto t = time(nullptr);
    auto tm = *localtime(&t);
    stringstream ss;
    ss << put_time(&tm, fmt);
    return ss.str();
}

/**
 * Standard command option processing, using global headerFile, numSampleRec, numSampleCol, delim, etc.
 * @param argc
 * @param argv
 * @return
 */
int getCmdOptions(int argc, char **argv) {
    opterr = 0;     // ignore unexpected option error
    int c;
    static struct option long_options[] = {
            {"verbose", no_argument,       &verbose_flag, 1},
            {"help",    no_argument,       nullptr,       '?'},
            {"escape",  required_argument, nullptr,       'e'},
            {"trim",    no_argument,       nullptr,       't'},
            {"Trim",    no_argument,       nullptr,       'T'},
            {"delim",   required_argument, nullptr,       'd'},
            {"header",  required_argument, nullptr,       'h'},
            {"recs",    required_argument, nullptr,       's'},
            {"cols",    required_argument, nullptr,       'n'},
            // {"shortoutput",  no_argument,       nullptr,       't'},
            // {"infileheader", no_argument,       nullptr,       '1'},
    };
    while (true) {
        int option_index = 0;
        c = getopt_long(argc, argv, "?tTh:d:s:n:e:", long_options, &option_index);
        // printf("getopt_long: 0x%0x\n", c);
        if (c == -1) break;     // end of processing
        switch (c) {
            case 0:
                // flag being not nullptr case, so *flag is set to be val, like above verbose_flag being set.
                break;
            case '?':
                return 1;
                // break;
            case 'h':
                headerFile = optarg;
                inFileHeader = false;
                break;
            case 's':
                numSampleRec = atoi(optarg);
                // if (numSampleRec < 1) numSampleRec = DefaultSampleRecs;
                break;
            case 'T':
                needTrim = false;
                break;
            case 'n':
                numSampleCol = atoi(optarg);
                // if (numSampleCol < 1) numSampleCol = DefaultSampleCols;
                break;
            case 't':
                needTrim = true;
                // shortOutput = true;
                break;
            case 'e':
                EscapeChar = optarg[0];
                break;
                // case '1':
                // inFileHeader = true;
                // break;
            case 'd':
                delim = optarg[0];
                break;
            case -1:
                break;
            default:
                return -1;  // not accepted input
        }
    }

    // while (optind < argc) printf("%s ", argv[optind++]);
    // following is to keep the initial logic
    if (optind < argc) delim = argv[optind][0];
    return 0;
}

/**
 * Get the n-th string from a vector, if n equals size, append one new empty string and return it
 * We assume n can't exceed size, so it should be called continuously, be careful
 * Note in C++, reference can be initialized, but then can't be reassigned, if you
 * do the assignment again, it's actually value copying, it still references to
 * the old variable through initialization.
 * So this function has to pass back a pointer
 * @param v
 * @param n
 * @return reference to the n-th string from the vector, starting from 0, till size of the vector
 */
inline string *get_nth(vector <string> &v, int n) {
    if (n >= v.size()) v.push_back("");
    return &v[n];
}

/**
 * Initialize all field related counters. Note this
 * can be called again and again, in case we have more and more fields
 * minfl, maxfl, fieldcount, fieldfillcount, fieldtotalbyts, sampleFields
 * all are vectors with same size, this function will add one more element
 * to each of them with proper initial value.
 * @param count
 */
inline void initializeFieldCounters(size_t count) {
    for (int k = minfl.size(); k < count; k++) {
        minfl.push_back(INT64_MAX);
        maxfl.push_back(0);
        fieldcount.push_back(0);
        fieldfillcount.push_back(0);
        fieldtotalbytes.push_back(0);
        if (needSampleCol) sampleFields.push_back({});
    }
}

/**
 * check if we should add current string element to the samples
 * The samples should have sampleCnt elements, so if it hasn't reached it yet,
 * just add the element.
 * Otherwise, suppose we now have a total number of elements, use a random
 * number to judge if we should add it to samples (replace one of them).
 * and if yes, use another random number to tell which one to replace.
 * looks like we should use move(element) for efficiency, but then after
 * return element is restored to empty (capacity very short), causing further performance issue.
 * I tried s1.swap(s2) or swap(s1, s2), but only one c_str() is reused, the other
 * got reallocated, so there's a copying, which is also a waste.
 * so we skipped the idea.
 * @param samples is the input vector of string as the samples
 * @param sampleCnt is the supposed size of the samples
 * @param element is the new one to be added or skipped
 * @param total is the current total number, it's already increased (including this element)
 */
inline void tryAddSample(vector <string> &samples, int sampleCnt, string &element, int64_t total) {
    static random_device dev;
    static mt19937 rng(dev());
    if (samples.size() < sampleCnt) samples.push_back(element);
    else {
        auto index = rng() % total;
        if (index < sampleCnt) samples[index] = element;    // should we use move() for efficiency? NO.
    }
}

/**
 * process input line, transforming from csv to tsv, into fields,
 * (this is copied from tsv2csv2.cpp, with just one change, skipping the pSubst use)
 * First, just process character by character,
 * if a field starts with double quote (except whitespace?), then it enters quote mode,
 * the quote mode will be ended with a matching double quote, note
 * the quote mode can be partial in a field.
 * if we need to trim, then trim all leading as we process, don't trim afterwards,
 * but at the end trim the endings, the complexity will be O(1).
 * @param line
 * @param fields
 * @return
 */
inline int processCsvFields(string &line, vector <string> &fields) {
    int fieldCnt = 0;
    bool gotNonBlank;   // true whenever: encountered a none whitespace
    bool quoteMode;     // true when leading (non-white?) character is a double quote, flipped by another standalone double quote

    int index = 0;
    size_t size = line.size();

    string *pfield = get_nth(fields, fieldCnt++);
    pfield->clear();
    // peek one to check if it's quoteMode
    // but first, if needTrim, let's trim first, this takes care of $',   "real thing" ...,' case.
    if (needTrim) {
        while (index < size) {
            if (isspace(line[index])) index++;
            else break;
        }
    }
    if (index < size && line[index] == Quote) {
        quoteMode = true;
        index++;
    } else quoteMode = false;
    gotNonBlank = false;

    while (index < size) {
        char cc = line[index++];
        if (quoteMode) {
            // be careful, now usually the EscapeChar is same as Quote
            if (cc == EscapeChar) {
                // if (!isspace(EscapeChar)) gotNonBlank = true;
                // peek one more character, see if we should escape
                if (index < size && line[index] == Quote) {
                    // yes, escape, consume it and advance
                    index++;
                    gotNonBlank = true;     // sure Quote is non-space, otherwise ...
                    pfield->push_back(Quote);
                } else {
                    // no escape, but it may be Quote itself
                    if (EscapeChar == Quote) {
                        // don't modify gotNonBlank yet, one trivial case
                        quoteMode = false;
                    } else {
                        gotNonBlank = true;     // sure EscapeChar is non-space, otherwise ...
                        pfield->push_back(cc);
                    }
                }
            } else {
                if (cc == Quote) quoteMode = false;
                else {
                    // take it
                    if (isspace(cc)) {
                        if (!needTrim || gotNonBlank) {
                            // if (cc == TAB) pfield->append(pSubst);
                            // else pfield->push_back(cc);
                            pfield->push_back(cc);
                        }
                    } else {
                        gotNonBlank = true;
                        pfield->push_back(cc);
                    }
                }
            }
        } else {
            // just charge on till separator, COMMA
            if (cc == COMMA) {
                // end of field, trimming (just end)
                if (needTrim)
                    pfield->erase(find_if(pfield->rbegin(), pfield->rend(), [](int ch) { return !isspace(ch); }).base(),
                                  pfield->end());
                // start new field
                pfield = get_nth(fields, fieldCnt++);
                pfield->clear();
                // peek one to check if it's quoteMode
                // but first, if needTrim, let's trim first, this takes care of $',   "real thing" ...,' case.
                if (needTrim) {
                    while (index < size) {
                        if (isspace(line[index])) index++;
                        else break;
                    }
                }
                if (index < size && line[index] == Quote) {
                    quoteMode = true;
                    index++;
                } else quoteMode = false;
                gotNonBlank = false;
            } else {
                // take it
                if (isspace(cc)) {
                    if (!needTrim || gotNonBlank) {
                        // if (cc == TAB) pfield->append(pSubst);
                        // else pfield->push_back(cc);
                        pfield->push_back(cc);
                    }
                } else {
                    gotNonBlank = true;
                    pfield->push_back(cc);
                }
            }
        }
    }

    // fix last field, trimming (just end)
    if (needTrim)
        pfield->erase(find_if(pfield->rbegin(), pfield->rend(), [](int ch) { return !isspace(ch); }).base(),
                      pfield->end());
    return fieldCnt;
}

/**
 * copied from processTsvFields, but simply just separate and construct fields
 * Turns out to be exactly the same as processTsvFields
 * let's use global delim instead of passing it
 * without any special caring.
 * @param line
 * @param fields
 * @return
 */
inline int processGeneralFields(string &line, vector <string> &fields) {
    int fieldCnt = 0;
    bool gotNonBlank;   // true whenever: encountered a none whitespace

    string *pfield = get_nth(fields, fieldCnt++);
    pfield->clear();
    gotNonBlank = false;

    int index = 0;
    size_t size = line.size();
    while (index < size) {
        char cc = line[index++];
        if (cc == delim) {
            // end of current field, fix the field, trimming (just end)
            if (needTrim)
                pfield->erase(find_if(pfield->rbegin(), pfield->rend(), [](int ch) { return !isspace(ch); }).base(),
                              pfield->end());

            // start new field
            pfield = get_nth(fields, fieldCnt++);
            pfield->clear();
            gotNonBlank = false;
        } else {
            if (isspace(cc)) {
                if (!needTrim || gotNonBlank) pfield->push_back(cc);
            } else {
                gotNonBlank = true;
                pfield->push_back(cc);
            }
        }
    }

    // fix last field, trimming (just end) and/or quoting
    if (needTrim)
        pfield->erase(find_if(pfield->rbegin(), pfield->rend(), [](int ch) { return !isspace(ch); }).base(),
                      pfield->end());
    return fieldCnt;
}

/**
 * process the fields to update minfl, maxfl, fieldcount and fieldtotalbytes vectors
 * Note fields will be destroyed if it's taken as a sample. So be careful!
 * @param fields
 * @param row_count the actual count of fields, not the size, since it's reused.
 */
inline void processFields(vector <string> &fields, int row_count) {
    for (int k = 0; k < row_count; k++) {
        size_t size = fields[k].size();
        if (minfl[k] > size) minfl[k] = size;
        if (maxfl[k] < size) maxfl[k] = size;
        fieldcount[k]++;
        if (!fields[k].empty()) fieldfillcount[k]++;
        fieldtotalbytes[k] += size;
        // vector <string> &samples = sampleFields[k];
        if (needSampleCol) tryAddSample(sampleFields[k], numSampleCol, fields[k], fieldcount[k]);
    }
}

int main(int argc, char **argv) {
    if (getCmdOptions(argc, argv) != 0 || (headerFile.empty() && !inFileHeader)) {
        printf(usage, argv[0]);
        return 1;
    }
    needSampleRow = numSampleRec != 0;
    needSampleCol = numSampleCol != 0;

    int (*pprocess)(string &, vector <string> &);
    if (delim == COMMA) pprocess = processCsvFields;
    else pprocess = processGeneralFields;

    time_t starttime, endtime;
    // chrono::time_point <chrono::system_clock> start = chrono::system_clock::now();
    time(&starttime);

    char hostname[_POSIX_HOST_NAME_MAX];
    gethostname(hostname, _POSIX_HOST_NAME_MAX);

    string line;
    vector <string> header;
    vector <string> fields;
    // read the header
    if (!inFileHeader) {
        ifstream hFile(headerFile);
        if (!hFile.is_open()) {
            ostringstream oss;
            oss << "failed to open the header file " << headerFile;
            perror(oss.str().c_str());
            return -1;
        }
        getline(hFile, line);
        hFile.close();
    } else {
        getline(cin, line);
    }
    pprocess(line, header);

    size_t fieldNum = header.size();
    // note for (auto f:header) still copies, it's also with for (auto &f:header).
    for (int k = 0; k < header.size(); k++) {
        if (header[k].empty()) {
            cout << "header has empty field, there's no need to process" << endl;
            return 0;
        }
    }
    initializeFieldCounters(fieldNum);

    // process input, line (record) by line (record)
    while (getline(cin, line)) {
        // process file size, line length min/max, line count
        size_t size = line.size() + 1;
        fsize += size;
        if (lmax < size) lmax = size;
        if (lmin > size) lmin = size;
        lcnt++;

        // explode line into fields
        int row_count = pprocess(line, fields);

        // process field per line min/max, total fields
        // size_t num = fields.size();
        if (minfpl > row_count) minfpl = row_count;
        if (maxfpl < row_count) maxfpl = row_count;
        fieldtotal += row_count;
        if (row_count > minfl.size()) initializeFieldCounters(row_count);
        // in case we have more fields

        // process each field, also gather the sample of fields.
        processFields(fields, row_count);

        // gather sample records.
        if (needSampleRow) tryAddSample(sampleRecs, numSampleRec, line, lcnt);
    }

    // let's shuffle the samples
    if (needSampleRow) shuffle(sampleRecs.begin(), sampleRecs.end(), mt19937(random_device()()));
    if (needSampleCol) {
        for (int k = 0; k < sampleFields.size(); k++)
            shuffle(sampleFields[k].begin(), sampleFields[k].end(), mt19937(random_device()()));
    }
    // duration time
    // chrono::time_point <chrono::system_clock> end = chrono::system_clock::now();
    // auto millis = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    time(&endtime);

    if (lcnt < 1) {
        cout << "There's no record at all" << endl;
    } else {
        printf("** Versium Analytics File Report **\n");
        cout << "Generation Time: " << formattedDT("%Y-%m-%d %H:%M:%S") << endl;
        printf("Processor: [%s]\n", hostname);
        printf("-----------------\n");
        printf("Lines\tBytes\n");
        printf("%lld\t%lld\n\n", lcnt, fsize);

        printf("** Line Length Report **\n");
        printf("------------------------\n");
        printf("Min\tMax\tAvg\n");
        printf("%lld\t%lld\t%-.02f\n\n", lmin, lmax, (double) fsize / lcnt);

        printf("** Field Inspection Report (FPL = Fields Per Line) **\n");
        printf("----------------------------------------------------\n");
        printf("MinFPL\tMaxFPL\tAvgFPL\n");
        printf("%lld\t%lld\t%-.02f\n\n", minfpl, maxfpl, (double) fieldtotal / lcnt);

        printf("** Field Content Report (Asterisk next to Ctr means field is same length in all lines; FR=Fill Rate) **\n");
        printf("-----------------------------------------------------------------------------------------\n");
        printf("FieldNum\tColName\tMinLen\tMaxLen\tAvgLen\tFRCtr\tFRPctg\tByteCtr\tBytePctg\n\n");

        int k;
        for (int i = 0; i < maxfpl; i++) {
            printf("%d%c\t", i + 1, minfl[i] == maxfl[i] ? '*' : ' ');
            if (i < header.size()) cout << header[i];
            cout << "\t";
            printf("%lld\t%lld\t%-.02f\t%lld\t%-.02f", minfl[i], maxfl[i], (double) fieldtotalbytes[i] / lcnt,
                   fieldfillcount[i], (double) fieldfillcount[i] / lcnt * 100.0);
            printf("\t%lld\t%-.02f", fieldtotalbytes[i], (double) fieldtotalbytes[i] / fsize * 100);
            if (needSampleCol) {
                cout << "\t<=>\t";
                k = 0;
                for (; k < sampleFields[i].size() - 1; k++) {
                    cout << sampleFields[i][k] << TAB;
                }
                if (k < sampleFields[i].size()) cout << sampleFields[i][k];
            };
            cout << endl;
        }

        if (needSampleRow) {
            cout << endl;
            k = 0;
            for (; k < header.size() - 1; k++) {
                cout << header[k] << TAB;
            }
            if (k < header.size()) cout << header[k];
            cout << endl;
            for (k = 0; k < sampleRecs.size(); k++) cout << sampleRecs[k] << endl;
        }
    }
    cout << endl << "Processing time: " << difftime(endtime, starttime) << " seconds." << endl;

    return 0;
}
