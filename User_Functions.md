# Introduction #

Overview of User-Defined-Functions (UDF).

# Details #

## File-Tree Overview ##

```
  user-functions  [for now just a place-holder]
  |- general
  |  \_ examples
  |- technology
  |  \_ .. (see below)
  |- config
  |- devices
  |- tools
  |- test
     |- test_parser
     |- test_functions
     |- test_UDF
```

  * **technology**: contains technolog dependent information like layer-mapping, DRC-rules, GDS-files of the standardlibraries, and much more.

```
    |- technology
       |- default.tll ... loads the default technology file (means techfile.tll)
       |- helper.tll  ... some general functions which are useful in the environment of technology
       |  
       |- <your-fab_process-option-ABC>
          \_ techfile.tll     ... master TLL-file. Load this to get a technology into your design. (needs a proper config/config.tll file)
          \_ drc.tll          ... DRC rules for generic devices
          \_ drc_lib_io.tll   ... DRC rules for I/O libraries (pads, esd, etc.)
          \_ drc_lib.tll      ... DRC rules for the digital standard-library
          \_ layer.tll        ... layer definitions
          \_ devices.tll      ... includes all devices 
          \_ gds_lib.tll      ... includes all standard-cells
          \_ gds_lib_io.tll   ... includes all IO-cells
          |- dev              ... folder contains devices
          |  \_ nmos.tll
          |  \_ pmos.tll
          |  \_ mim.tll
          |- GDS               ... GDS-files
          |  |- lib            ... standard cells
          |  |  \_ nand_1x.gds
          |  |- lib_io         ... I/O-cells
          |  |  \_ ana_100.gds
```

  * **config**: folder contains mainly sample configuration files. The user
will have his own configuration files placed somewhere in his home. Since
these files are kind of templates the full power of configuration functionality
should be visible.

  * **devices**: contains devices which were not provided by the designkit of the
fab. Nontheless, their TELL-Description depends on some technology which
implies that the interface to DRC-rules has to be standardized.

## GENERAL ##

### definitions.tll ###
### helper.tll ###
### geometry.tll ###
### wires.tll ###
### wires\_ortho\_dia.tll ###

## CONFIG ##

### config.tll ###
### config\_colors.tll ###
### config\_colors\_rgb.tll ###
### config\_pattern.tll ###
### config\_misc.tll ###

## TEST ##

### test.tll ###

## TOOLS ##

### Logos ###

## TECHNOLOGY ##

### default.tll ###
### helper.tll ###

### Technology-Directory (fab-process-name) ###

#### techfile.tll ####
#### drc.tll ####
#### layer.tll ####

#### Devices (dev) ####