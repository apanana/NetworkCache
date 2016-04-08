# NetworkCache

### How to run:
* `make` creates executables for the server, direct_client, and testing_client
* `make runserver` starts the server
* `python3 run_test.py ./out/test_client` runs the testing scripts on the cache. Adding a number 0-31 will run a specific test.

### Testing:
Initially I set up a `direct_client` that could take user input from the command line to test the first TCP implementation. Eventually after the testing scripts were properly ported over, I tested both TCP and UDP against the full testing suite. A few tests have been omitted because of ambiguity regarding whether or not they are part of the spec and relevancy to our RESTful API.
I made a bunch of patches to the testing suite so that the tests that I did run adhered to the API. This basically meant making sure that nothing was ever treated as an INT, or in other words, ensuring that all inputs to the client were strings.

### TCP/UDP:
Currently both TCP and UDP are working and have been tested. I put the respective functions for either protocol into seperate modules to keep things clean.
TCP is over port number 3490.
UDP is over port number 4950.
