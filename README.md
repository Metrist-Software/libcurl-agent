# Metrist libcurl agent

Metrist provides various language-specific agents to help in discovering outgoing API dependencies and providing the
data for in-process monitoring.

The agent in this repository is a bit different: it hooks into the widely used [curl](https://curl.se/libcurl/) library and so provides for
in-process monitoring for any stack that relies on libcurl for communicating with APIs.

This agent can be used for the following languages: PHP, C/C++, and any other use of libcurl.

## Restrictions

We can only hook into libcurl if it is used as a dynamic library, and (currently) only on Linux because we use
functionality that is specific to the Linux ELF linker/loader.

Currently, we expect libcurl to be used in "easy" mode. If you have a different use case, please contact us through
your regular support channels.

## Installation

You can obtain the latest binary version from our distribution site:

... TODO

or build from source (see below). Install the library in a convenient location and then point to it
using the environment variable `LD_AUDIT` before starting the software to be monitored. This will
cause the Linux loader to load the agent before other libraries and allows it to intercept calls to
libcurl.

## Configuration

Example: PHP 7.4 running with FPM behind Nginx under Systemd. Do

```
sudo systemctl edit php7.4-fpm.service
```

and make sure that the configuration override contains:

``` sh
[Service]
Environment="LD_AUDIT=/opt/canary/libcanary_curl_ipa.so.0.1"
```

Then restart the service and monitoring should commence. There are two more environment
variables you can use to change where the telemetry data is sent to:

* `METRIST_AGENT_HOST` can be set to an IP4/6 address to send the telemetry data to, the default
  is to send to localhost. Note that due to limitations of what an audit library can do, the use of
  hostnames is currently not possible here.
* `METRIST_AGENT_PORT` can be set to a port to send the telemetry data to, the default is to
  sent to port 51712.


## Building

You need the standard Unix toolchain (`make`, `gcc`) and the development version of
libcurl installed. On a Ubuntu machine, this is usually accomplished by invoking:

``` sh
sudo apt install build-essential libcurl4-openssl-dev
```

Invoking `make` will then build the library and run a test suite over the included
linked list library. The agent is now in your directory as a `.so` file.
