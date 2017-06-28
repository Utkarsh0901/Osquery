# OSquery

<p align="center">
<img align="center" src="https://raw.githubusercontent.com/Utkarsh0901/Osquery/master/OSquery_logo.png" alt="osquery logo" width="500"/>

<p align="center">
osquery is an operating system instrumentation framework for OS X/macOS, Windows, and Linux. <br/>
The tools make low-level operating system analytics and monitoring both performant and intuitive.

| Platform | Build status  | | | |
|----------|---------------|---|---|---|
macOS 10.12    | [![Build Status](https://jenkins.osquery.io/job/osqueryMasterBuildOSX10.12/badge/icon)](https://jenkins.osquery.io/job/osqueryMasterBuildOSX10.12/) | | **Homepage:** | https://osquery.io
CentOS 6.x | [![Build Status](https://jenkins.osquery.io/job/osqueryMasterBuildCentOS6/badge/icon)](https://jenkins.osquery.io/job/osqueryMasterBuildCentOS6/) | | **Downloads:** | https://osquery.io/downloads
CentOS 7.x   | [![Build Status](https://jenkins.osquery.io/job/osqueryMasterBuildCentOS7/badge/icon)](https://jenkins.osquery.io/job/osqueryMasterBuildCentOS7/) | | **Tables:** | https://osquery.io/tables
Ubuntu 14.04   | [![Build Status](https://jenkins.osquery.io/job/osqueryMasterBuildUbuntu14/badge/icon)](https://jenkins.osquery.io/job/osqueryMasterBuildUbuntu14/) | | **Packs:** | https://osquery.io/packs
Ubuntu 16.04 | [![Build Status](https://jenkins.osquery.io/job/osqueryMasterBuildUbuntu16/badge/icon)](https://jenkins.osquery.io/job/osqueryMasterBuildUbuntu16/) | | **Guide:** | https://osquery.readthedocs.org
Windows 2016 | [![Build Status](https://jenkins.osquery.io/job/osqueryMasterBuildWindows2016/badge/icon)](https://jenkins.osquery.io/job/osqueryMasterBuildWindows16/) | | [![Slack Status](https://osquery-slack.herokuapp.com/badge.svg)](https://osquery-slack.herokuapp.com) | https://osquery-slack.herokuapp.com
Windows 10 | [![Build Status](https://jenkins.osquery.io/job/osqueryMasterBuildWindows10/badge/icon)](https://jenkins.osquery.io/job/osqueryMasterBuildWindows10/) | | |

There are many additional [continuous build jobs](https://jenkins.osquery.io/) that perform **dynamic** and **static** analysis, test the **package build** process, **rebuild dependencies** from source, assure **deterministic build** on macOS and Linux, **fuzz** test the virtual tables, and build on several other platforms not included above. Code safety, testing rigor, data integrity, and a friendly development community are our primary goals.

# Platform : Ubuntu 16.04 -
## Downloads 

For latest stable builds for OS X (pkg) and Linux (deb/rpm), as well as yum and apt repository information visit [https://osquery.io/downloads](https://osquery.io/downloads/). Windows 10, 8, Server 2012 and 2016 packages are published to [Chocolatey](https://chocolatey.org/packages/osquery).

```bash
$ git clone http://github.com/facebook/osquery.git
$ cd osquery/
```
## Building from source

Building osquery from source is encouraged! [Check out the documentation](https://osquery.readthedocs.org/en/latest/development/building/) to get started and join our developer community by giving us feedback in Github issues or submitting pull requests!

We *officially* support a subset of OS versions for **building** because it is rather intense.
- Ubuntu 14.04 and 16.04, CentOS 6.5 and 7
- Apple macOS 10.12
- Windows 10 and Server 2016

```bash
$ make deps
$ make -j 8
$ ./build/<platform>/osquery/osqueryi
```
- We will use linux platform. You can look in _osquery/build_ for more supporting platforms. 
## Optional Dependencies
- __FPM__ : http://midactstech.blogspot.in/2014/05/install-fpm.html
- __DOCKER__ : https://docs.docker.com/engine/installation/#get-started
## Making and using  Plugins
- Add __filesystem_logger2.cpp__ to _osquery/osquery/logger/plugins/_ .
- This plugin is use for logging as well as sending logs to remote server.
-  Add this plugin_name to _osquery/osquery/logger/CMakeLists.txt

```
osquery/osquery/logger/CMakeList.txt
...
file(GLOB OSQUERY_LOGGER_TESTS "tests/*.cpp")
ADD_OSQUERY_TEST_CORE(${OSQUERY_LOGGER_TESTS})

set(OSQUERY_LOGGER_PLUGINS
  "plugins/buffered.cpp"
  "plugins/filesystem_logger.cpp"
  "plugins/filesystem_logger2.cpp"                                 # here
  "plugins/tls_logger.cpp"
  "plugins/stdout.cpp"
)

ADD_OSQUERY_LIBRARY_ADDITIONAL(osquery_logger_plugins ${OSQUERY_LOGGER_PLUGINS})
...

```
## Linking Library
- __filesystem_logger2__ is using libcurl library. Hence, for linking this library we have to add it to _osquery/osquery/tables/CMakeLists.txt_
```
osquery/osquery/tables/CMakeLists.txt
...
# Linux specific and applicable tables.
if(LINUX)
  set(TABLE_PLATFORM "linux")

  ADD_OSQUERY_LINK_ADDITIONAL("libresolv.so")
  ADD_OSQUERY_LINK_ADDITIONAL("cryptsetup devmapper")
  ADD_OSQUERY_LINK_ADDITIONAL("gcrypt gpg-error")
  ADD_OSQUERY_LINK_ADDITIONAL("blkid")
  ADD_OSQUERY_LINK_ADDITIONAL("ip4tc")
  ADD_OSQUERY_LINK_ADDITIONAL("libcurl.a")                         # here

  ADD_OSQUERY_LINK_ADDITIONAL("apt-pkg dpkg lzma lz4 bz2")
  ADD_OSQUERY_LINK_ADDITIONAL("rpm rpmio beecrypt popt db")
endif()

if(POSIX)
  ADD_OSQUERY_LINK_ADDITIONAL("augeas fa xml2")
endif()
...
```
## Configuring OSqueryd
- We can configure OSquery by using __osqueryd2.conf__. You can specify which plugin to use, where to produce logs, ip address of the remote server, and many other things. When starting osqueryd you may use __--logger_plugin=name__ where the name is the string identifier used in REGISTER ( for filesystem_logger2 use __filesystem2__ ). For using __osqueryd2.conf__ you have to use --config_path="/path/to/config/osqueryd2.conf" while running osqueryd.
## Running osqueryd
```bash
$ sudo ./build/xenial/osquery/osqueryd --config_path="/path/to/config//osqueryd2.conf" --allow_unsafe
```
