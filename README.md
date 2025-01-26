# WebLinkExtractor
## About the program

Console utility for extracting links (and links from links) from a webpage recursively.
The project was created mainly for learning purposes, fun and exploration of what's possible with modern C++ standards and features.

Throughout the journey I went through:
- proper way to write multithreaded programs
- atomic variables and operations
- different synchronization mechanisms for multithreaded programs (like mutexes, semaphores, condition variables)
- writing my own threadpool implementation
- basic and advanced SQL operations (PostgreSQL via [SOCI](https://soci.sourceforge.net/doc/master/))
- [libcurl](https://curl.se/libcurl/)
- integrating python into C++ (and the other way around) using standard C Python bindings and pybind11
- regular expressions (regex)

## Features
- like every decent console program has input flags to customize number of threads, parsing depth, etc.. Simply run it `./build/projectA` or `make run` to see the prompt
- includes possibility to write out parsed link data to the database
- you may want to uncomment a block of code in `main.cpp` to have a list of all links alive being put to the console

### Setup PostgreSQL locally
I assume you have already installed postgres with package manager, initialized the cluster, launched the database service, and have a dedicated user (in my case user1) on your local machine. There are lots of guides how to do that specifically for you on the Internet.

To make it actually usable you should go through a few steps:
1. connect to the database with "root" privileges
`sudo -u postgres psql`
2. create a database in postgres prompt
`CREATE DATABASE projectA;`
3. check if database was created successfully
`SELECT datname FROM pg_database;`
4. grant your user rights to connect to it
`GRANT CONNECT ON DATABASE projectA TO user1;`
5. connect to newly created database
`\c projecta`
6. allow some operations to user in the database
`GRANT CREATE ON SCHEMA public TO user1;`
`GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO user1;`
7. exit psql with `\q`
8. run the `./build/projectA` with proper flags and have database populated
9. connect back to the sql with your user
`psql -U user1 -d db1 -h localhost -p 5432`
10. see the data in your table there
`SELECT * FROM your_new_table`


## Working program

![gif](https://github.com/Andriy-Bilenko/projectA/raw/main/res/working.gif)





If you ran into some issues by any chance or need to contact the developer, it would be great to recieve your valuable feedback on email: *bilenko.a.uni@gmail.com*.

<div align="right">
<table><td>
<a href="#start-of-content">↥ Scroll to top</a>
</td></table>
</div>
