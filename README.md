# projectA
# In progress
### program idea
web crawler using curl crawling set depth of links from the starting link specified by user
(maybe used boost asio)
I want everything concurrent and having connected to postgreSQL via soci

So:
- parsing htmls and getting links from them
    - checking for cyclic links and duplicates
- parse concurrently with a mutex on resulting vector of links
- put those links to SQL based on link depth right when we got the link and output to the console


// 1) psql -U postgres -d db1 -h localhost -p 5432
// 2) CREATE DATABASE projectA
// 3) check database was created:"SELECT datname FROM pg_database;"
// 4) GRANT CONNECT ON DATABASE projectA TO user1;
// 5) \c projecta
// 6) GRANT CREATE ON SCHEMA public TO user1;
// 7) GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO user1;
// 8) exit psql with "\q"
// 9) run code
// 10) psql -U user1 -d db1 -h localhost -p 5432
// 11) run "SELECT * FROM your_new_table" and see the data there

