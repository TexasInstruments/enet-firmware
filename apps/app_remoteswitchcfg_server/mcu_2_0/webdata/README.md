## Example Summary

This application demonstrates how to build an HTTP server. The application
provides an example of one possible way to serve static pages from the
Network Services HTTP server.

## Example Usage

* Run the example. It begins by starting the network stack. When the stack
receives an IP address from the DHCP server, the IP address is written to the
console. **Note the IP address received.**

## Application Design Details

* This application has one primary task:
    * 'serverFxn' creates the HTTP Server and then starts the server by calling
    the HTTPServer_serveSelect() function. This function accepts new connections
    to the server and calls methods to process the incoming requests.

* The example urlhandler provided in `urlsimple.c` is just one example of how a
urlhandler can be designed. From the error code returned in a given
situation, to the status returned to the server after a request that
determines how the server proceeds - these are design decisions influenced by
the HTTP standard.

* `urlmemzip.c` provides an additional example of a urlhandler. This urlhandler
utilizes `memzip.c`, which relies on `make-memzip.py`, to ultimately serve the
files found in `webpages`.

### Serving Static Pages

The example shown here is _one_ way of serving static web pages from an
HTTPServer application. There are numerous other ways to accomplish this task.

* `make-memzip.py` takes in a directory of files to be served by the
web server. It produces a zip file containing all of the files, still
uncompressed. The zip file is then converted into an array (`memzip_data`)
which can be seen in the output file `memzip_files.c`. All of the data
contained in this zip file, along with bookkeeping information used by
`memzip.c`, is contained in this array.

* `memzip.c` provides the function `memzip_locate` which locates a given
filename inside `memzip_data` and fills a pointer `pData` with the
original file's content. This pointer becomes the body of HTTP responses
crafted by `urlmemzip.c` in `URLMemzip_process`.

* To utilize the functionality provided by `urlmemzip.c`, create a directory
containing web page files and other server resources - similar to the
_targetfs_ directory in this example. The `make-memzip.py` script can then
be run to prepare the file contents to be served by the server.

    * For usage details run
        ```
        python make-memzip.py -h
        ```

    * If using CCS, the provided Pre-build step should be edited with the path
    of a local Python3 installation and the path of the directory containing
    the files to be served.
        ```Python
        <Path to Python executable> ${PROJECT_LOC}/make-memzip.py -c ${PROJECT_LOC}/memzip_files.c <Path to static resources>
        ```
## Acknowledgements

The memzip files (memzip.h, memzip.c, and make-memzip.py) were sourced from
https://github.com/micropython/micropython (c3095b37e96aeb69564f53d30a12242ab42bbd02)
and slightly modified by Texas Instruments. Texas Instruments also added the
MIT license to the top of these files in an effort to clarify their licensing.

