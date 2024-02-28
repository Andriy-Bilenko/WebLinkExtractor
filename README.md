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
