# csvsuite
Python's csvkit written in C++

IT IS UNDER CONSTRUCTION NOW.

## csvsuite (ala Python's csvkit)
*    About
*    [FAQ](#faq)
*    [CSV kit](#CSV-kit)
*    [Statistics performance](#statistics-performance)
*    [Sorting performance](#sorting-performance)
*    [SQL performance](#sql-performance)
*    [Build All](#build-all)


### About
Description goes here.

### FAQ

> Why?


### CSV kit

Shortly: Small csv kit is shortened Python's csvkit with the speed of Rust's xsv (and even faster). It is written on top of the
csv_co "parse engine" and, where needed, with the help of task-based parallelism library transwarp. It tries to reproduce the
functionality of the csvkit whenever possible.

<h4>Note: small csv kit is in an early development stage and with code needing some revision. But it is already usable. 
Bug reports, scenarios that failed and so on are very welcome.</h4>  

The goals for the reproduction were: to find out the complexity and limitations of the applicability of the C++ ecosystem for broad
universal tasks, where Python is good with its rich environment for data processing, encoding, localization, and so on. It was also
interesting to see the performance benefits of C++ applications in non-traditional areas.  
These utilities are currently implemented: csvclean, csvcut, csvgrep, csvjoin, csvjson, csvlook, csvsort, csvstack, csvstat. 

#### Restrictions:  

1) Things not implemented for now: in2csv, sql2csv, csvformat, csvpy, csvsql.

2) Due to the fact that csv_co engine only supports RFC 4180, the following utility arguments are missing:
	- <i>-d DELIMITER, --delimiter DELIMITER</i>  
	- <i>-t, --tabs</i>   
	- <i>-q QUOTECHAR, --quotechar QUOTECHAR</i>    
	- <i>-u {0,1,2,3}, --quoting {0,1,2,3}</i>  
	- <i>-b, --no-doublequote</i>  
	- <i>-y SNIFF_LIMIT, --snifflimit SNIFF_LIMIT</i>  
	The remaining arguments must be present. Another thing is that they may accidentally remain unrealized at the moment.
        When running, any utility tries to quickly check the equality of columns in your documents to match RFC 4180.
        To ensure a document is valid there is special csvclean utility in csvkit and in here. 

3) When handling date and datetime data types and their localization, csvkit relies on the rich Python datetime library. It also
allows you to work with time representations such as 'yesterday', 'today', 'tomorrow', and so on. However, our tool in this sense
is tightly bound to the --date-format and --datetime-format arguments and works well only on those platforms where this is
supported by the environment/compiler/standard library. And the --date-lib-parser argument engages the special date library to
improve the situation and ensure consistency everywhere. For more info see tests located in csvkit_core_test.cpp.

4) Some utilities may not support composition/piping.
5) csvjson does not support geometry option for right now. 

### Statistics performance
There were measured the performances of three tools: [csvkit(1.5.0)'s csvstat](https://pypi.org/project/csvkit/), 
[xsv(0.13.0)' stats](https://github.com/BurntSushi/xsv/releases/tag/0.13.0) and this one (csv_co's csvstat) at files: 
crime.csv, worldcitiespop.csv, flights.csv and question_tags.csv with (or with no) a limited number of columns
(so as not to break up screenshots). Here are the results (csvkit, xsv, and this tool screenshots):  

<h3>(crime.csv)(csvkit)</h3> 
 
![image info](./img/crime_csvkit.png) 
-
<h3>(crime.csv)(xsv)</h3>  

![image info](./img/crime_xsv.png) 
-
<h3>(crime.csv)(this tool)</h3>
  
![image info](./img/crime_csv_co.png) 

Here, xsv is the winner. It produces results in less than a second.  
The peculiarity of this file is that it is not utf8-encoded and that its fields are heavily quoted.  

<h3>(worldcitiespop.csv)(csvkit)</h3>

![image info](./img/worldcitiespop_csvkit.png)
-
<h3>(worldcitiespop.csv)(xsv)</h3>

![image info](./img/worldcitiespop_xsv.png)
-
<h3>(worldcitiespop.csv)(this tool)</h3>

![image info](./img/worldcitiespop_csv_co.png)

Here, this tool is even faster. 
Note: since xsv does not calculate 'most decimal places' in its statistics, unlike csvkit, we have disabled this time-consuming 
option at ours.

<h3>(flights.csv)(csvkit)</h3>

![image info](./img/flights_c_1_10_csvkit.png)
-
<h3>(flights.csv)(xsv)</h3>

![image info](./img/flights_c_1_10_xsv.png)
-
<h3>(flights.csv)(this tool)</h3>

![image info](./img/flights_c_1_10_csv_co.png)

Here we again beat xsv by more than 2 times by doing statistics on the first 10 columns.
If we did full statistics on all columns, xsv would run out of memory on our current machine (now we had 12 GB RAM), 
as in the following test.

<h3>(question_tags.csv)(xsv)</h3>

![image info](./img/question_tags_xsv.png)
-
<h3>(question_tags.csv)(this tool)</h3>

![image info](./img/question_tags_csv_co.png)

Our tool gives the result in about 42 seconds. Here we could not wait for the result from csvkit within a reasonable time. 
Thus, both csvkit and xsv are unable to produce their results where for csv_co' csvstat the reasons why this is not possible 
are not the case. This is a subject for further research.

### Sorting performance
If we talk about sorting by columns of string types, then the xsv is unrivaled, far ahead of other means in time, because
it obviously uses efficient algorithms for sorting strings, and without the use of parallelism. However, let's see how
effective it is to sort a group of columns where there is one numeric type (the -N option is required, otherwise the
results will be incorrect). We only need about 7 seconds versus 21 at the xsv. The csvkit is more than a minute behind us.

![image info](./img/sort_worldcitiespop.png)

### SQL performance
C++ is said to outperform Python in general, non-specialized areas by about 3 times. In this case, it is so. And even
better. In light of the impossibility of parallelizing the filling of the database table using language tools.
Here it turned out 4 times faster, because the csvkit spends a significant part of the time determining column types,
unlike our tool.

![image info](./img/csvsql_worldcitiespop.png)

### Build All

<h4>Note. For now, you need Python installed. This is to configure one of libraries used. Over time, this dependence will be removed.</h4>

_Conventional:_
```bash
mkdir build && cd build
cmake ..
make -j 4
```

_Better (if you have clang, libc++-dev, libc++abi-dev packages or their analogs installed):_
```bash
mkdir build && cd build
cmake -DCMAKE_CXX_COMPILER=clang++ -D_STDLIB_LIBCPP=ON ..
make -j 4
```

_Best (if you simply have got clang):_
```bash
mkdir build && cd build
cmake -DCMAKE_CXX_COMPILER=clang++ -D_STDLIB_LIBCPP=OFF ..
make -j 4
```

_Check for memory safety (if you have clang sanitizers packages installed):_
```bash
mkdir build && cd build
cmake -DCMAKE_CXX_COMPILER=clang++ -D_SANITY_CHECK=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j 4
```

_MSVC (in x64 Native Tools Command Prompt):_  
```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release ..
msbuild /property:Configuration=Release csv_co.sln
```
_Running tests in MSVC:_
```bash
cd build/test
```
_Then, run all tests:_
```bash
ctest.exe -j 6 
```
_Or run a particular test (still staying in the test directory):_
```bash
Release/test.exe
```
NOTE! The code for the **csvjoin** and the **csvjson** utilities is currently very problematic for
the Microsoft Compilers of certain versions. So, for now, those utilities are made not fully
functional here. ;-(
