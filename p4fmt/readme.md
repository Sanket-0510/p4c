This PR is the demonstration of simple p4c backend with minimal frontend passes, printing the formatted p4c program using the pretty-printer. Use can pass a `.p4` file to this `./p4fmt` binary and it will print the formatted p4 program for you.

to use pretty-printer clone this repo

```
1. git clone github.com/Sanket-0510/p4c

2. git checkout origin/feature-pretty-printer

3. cd build/p4fmt

4. cmake ..

5. make

```

This will build the p4fmt binary in the build folder.

to use this binary 

```
cd p4c/build/p4fmt
```
To print formatted output to the console 

To print output to the different file

`./p4fmt ../../formatter_sample -o <file_name>`

eg. `./p4fmt ../../formatter_sample -o ../../formatter_sample/sample_txt` 

I have also added some useful options like
```
--with-frontend
--with-midend
--print-IRnodes
```


I have added a sample p4 program in example.p4 but you can pass any p4 file to this binary and it will format the code for you.

