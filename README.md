# paparazzi-unittest
Contains unittest code for paparazzi airborne software

This is a very first attempt to introduce unit testing to paparazzi. It was succesfully used during development of the sdcard_spi and sd_logger_spi_direct. Some modifications were required to blend it into the existing paparazzi environment. For more information about unity and cmock, please refer to http://www.throwtheswitch.org/.

# Setup of test-environment
* Put this repository parallel to [paparazzi](https://github.com/paparazzi/paparazzi), for example: ~/paparazzi and ~/paparazzi-unittest
```
$ cd ~
$ git clone https://github.com/bartslinger/paparazzi-unittest.git
```
* Download also repository [paparazzi-tools](https://github.com/bartslinger/paparazzi-tools)
```
$ git clone https://github.com/bartslinger/paparazzi-tools.git
```
* Call the configure_workspace.py script in tools:
```
$ python ~/paparazzi-tools/qtcreator_ide_config/configure_workspace.py <AC_NAME> ~/paparazzi-unittest/yml_template.txt
```
\<AC_NAME\> = Aircraft name as defined in conf.xml, for example "Microjet" (without quotes)
