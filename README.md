# nzProx & nzServ
For CS441 (spring 2016).

[Link to Google Slide presentation](https://docs.google.com/presentation/d/1prJkG12D6sebPpJn3dmrYzR_8g3vMrfD6E460B9rLMk/edit?usp=sharing).

## Setup
Running `setup_Ubuntu14-04.sh` should get you most of the way there. The script asks you which ports you want to use and whether you are running from the server(s) or proxy and sets everything up for you. All you have to do is edit `index.html` if you want the server(s) to display different content. Check the `Makefile` also.

### nzServ
On the web server(s), run `make nzServ` to build the server. Make sure that you run `nzServ.out` from the `public_html` folder. It is run using:

`../nzServ.out PORT DIR`

Most of the time `DIR` can just be `.`. Example:

`../nzServ.out 80 .`

### nzProx
On the proxy, run `make nzProx` to build the proxy. Currently nzProx is set up to support exactly 2 web servers (no more, no less). It is run using:

`./nzProx.out -l PORT -h HOST0 -j HOST1 -p PORT [-f]`

HOST0 and HOST1 are the two web servers, `-l` signifies the local port that the proxy listens on, and `-p` signifies the port that the proxy forwards to on the servers.

Example:

`./nzProx.out -l 80 -h 192.168.1.100 -j 192.168.1.101 -p 80 -f`

## Stress Tests
If you want to use the stress tests, you'll need [Locust](http://locust.io/) and [Beautiful Soup](https://www.crummy.com/software/BeautifulSoup/). Just run `STRESS.sh` from your local machine with the address of the proxy as the only argument and it will do the rest for you.

## Hosting
I used [Digital Ocean](https://m.do.co/c/4f2d437b006f) to host these applications for demonstration.
