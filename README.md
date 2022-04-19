
# shell-file-tools
A collection of simple, fast, and efficient command-line tools for data manipulation.



# Requirements
- GNU Make



# Installation
Clone the repository to the desired location and run the following commands:
```
git clone https://github.com/versiumanalytics/shell-file-tools
cd shell-file-tools/
./configure
make
make install
```
Running `make install` will copy the compiled binaries in the *shell-file-tools/src* directory over to the default *bindir* for your system. To change the default location see `./configure --help`.

Note: You may need to run `make install` with sudo if you do not have write permissions for the *bin* directory.
```
sudo make install
```

To clean up your source tree after installing run `make clean`




# Usage Guide


## fld-ctr
Prints output with a field number prefixed to each field encapsulated in parenthesis. Each line starts with the number of fields it sees. start-idx allows you to have it prefix the fields starting with a value other than '0'. This is useful as some tools reference fields starting from 0, while others start from 1.

### Usage: 
```
fld-ctr [delim-chars] {start-idx} < infile > output
```

### Example:
```
$ head -3 test
0,b
1,b
2,a

$ head -3 test | fld-ctr ','
2:(0:0) (1:b)
2:(0:1) (1:b)
2:(0:2) (1:a)

$ head -3 test | fld-ctr ',' 1
2:(1:0) (2:b)
2:(1:1) (2:b)
2:(1:2) (2:a)
```


## fslicer
"Virtually" slices a file on disk into X number of slices and outputs slice Y to standard output. Extraordinarily useful in parallel processing of a single large file.

### Usage:
``` 
fslicer {opts} [infile] [num-slices] [slice-to-dump]
    {opts} :
           --header - show header each slice.
           --headerfile=xxxx - replicate the contents of headerfile on top of each slice. max 128k

  num-slices      how many slices to make of the file
  slice-to-dump   which slice number to dump (0 .. num-slices - 1)
  ```

### Example:
Run 24 processes in parallel, doing a word count for each slice of the file. Output the results
of each slices' word count to a separate file. Consider this the 'map' stage of a map-reduce job.
Summing up the results of those files afterwards would be the 'reduce' stage.
```
$ set SLICES = 24
$ set SEQSLICES = `expr $SLICES - 1`
$ set BIGFILE = '/mnt0/this-file-is-huge'

$ for x in $(seq -w 0 $SEQSLICES)
> do 
> nohup fslicer $BIGFILE $SLICES $x | wc > wc-slice$x.txt &
> done
```


## fwc

"Fast Word Count". Similar to "wc", this tool samples a file and makes a projection as to approximately how many lines are in the file. Should be within 99% accuracy, but runs quickly even on very large files. Note this is a probabalistic estimate of the number of lines, not actually a count.

### Usage:
```
fwc [file]
```

### Example:
```
$ time seq -f '%-10.0f' 1 250000000 > /tmp/test
177.494u 3.527s 3:01.53 99.7% 10+172k 12+21329io 0pf+0w

$ du -h /tmp/test
2.6G /tmp/test

$ time wc /tmp/test
 250000000 250000000 2750000000 /tmp/test
29.731u 1.661s 0:31.39 100.0% 10+172k 0+0io 0pf+0w

$ time fwc /tmp/test
250000000
0.000u 0.025s 0:00.02 100.0% 12+332k 0+0io 0pf+0w
```


## get-fs
Reads an entire file and builds a digest of fill rates and other summary stats. Note that the output is semi-tab delimited so importing (copy/paste or actual import) won't look too terribly ugly.

### Usage:
```
get-fs [-d delim] [-t | -T] [-e escape char] [-h HEADER-FILE.txt] [-s SAMPLE-SIZE] [-n SAMPLE-COLUMNS-NUM]
  -d -- delimiter, if omitted, default to be tabular
  -e -- escape character for csv, default to be double quote
  -t -- tells the tool to trim fields.
  -T -- tells the tool to not trim fields.
  -h -- read headerfile (tab separated or newline separated) and inject field names into output. Omitting this means first line is header.
  -s -- include sample records at bottom of report.
  -n -- include column samples on right of report.
```

### Example:
```
$ head -10000 contacts.csv | get-fs ',' -h contacts-header.

** File Report **
-----------------
Lines Bytes
10000 6575128

** Line Length Report **
------------------------
Min Max Avg
500 7328 657.51

** Field Inspection Report (FPL = Fields Per Line) **
----------------------------------------------------
MinFPL MaxFPL AvgFPL
393 393 393.00

** Field Content Report (Asterisk next to Ctr means field is same length in all lines; FR=Fill Rate) **

-----------------------------------------------------------------------------------------

FieldNum ColName MinLen MaxLen AvgLen FRCtr FRPctg ByteCtr BytePctg

1 FirstName 1 15 5.93 10000 100.00 59253 0.90
2 MiddleName 0 19 0.70 6993 69.93 7020 0.11
3 LastName 2 20 6.63 10000 100.00 66283 1.01
4 Address 7 31 14.66 10000 100.00 146614 2.23
5 Unit 0 4 0.00 1 0.01 4 0.00
6 City 4 13 4.79 10000 100.00 47920 0.73
7 State 2 5 2.00 10000 100.00 20006 0.30
8 Zip 0 5 5.00 9999 99.99 49993 0.76
```


## hashpend

Reads a hash table then builds keys from input records specified by fields on cmd line and appends any result to the end. Particularly useful as a replacement to "join" when you have a large, unsorted file and want to add values from a lookup table (e.g. zip -> state mapping).

### Usage:
```
hashpend {opts} [tab-delimited-hash-table] {fields ...}
   {opts} :  
          -i  x     inject inline at field x, instead of appending
          -pi x     pad x fields for unmatched inline injection
          -r  x     replace inline at field x, instead of appending
          -r0 x     replace field INLINE, ONLY if 0/null
          -l        force all keys to lowercase first
          -p  x     pad unmatched records with x blank fields.
          -d  xyz   delimit each field from input with xyz.
          -o0       EXCLUDE matching rows from output.
          -o1       INCLUDE ONLY matching rows from output.
          -v        verbose - debug info.

    tab-delimited-hash-table contains [key]|data|data|data which is appended when
    fields (from 0) concatenated match
      
    data elements will be APPENDED to each input record.
```
### Example:
```
$ cat test/hp1-map

b r1 r2 r3
bc x1 x2 x3


$ cat test/hp1-test

a b c
1 b
2 c b
3 b c


$ cat test/hp1-test | hashpend -p 3 test/hp1-map 1 2

a b c x1 x2 x3

1 b r1 r2 r3

2 c b

3 b c x1 x2 x3
```


## rnd-extract

Randomly extracts n lines from a file. This is done using a specific seek and read mechanism that gives a true random sample of a file. This is very useful for generating file statistics or understanding what data is in the file aside from the top and bottom which are notorious for having garbage. Since this is a random seek/read, this is only useful up to maybe 1m lines for large files. Otherwise, use the GNU "sample" command.

### Usage:
```
rnd-extract [file] [#lines]
```

## Example:
```
$ seq 0 100 > /tmp/test
$ rnd-extract  /tmp/test 5
80
41
70
23
30
```