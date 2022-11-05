
# OpenVCB

openVCB is an open source implementation of the Virtual Circuit Board reader, assembler, and simulation engine. 
The goal of this project is to drive the community study and development of high performance simulations for the game. 

Get Virtual Circuit Board at
https://store.steampowered.com/app/1885690/Virtual_Circuit_Board/

## Currently implemented (Aug/17/2022)
* **A somewhat optimized event based simulation engine which is anywhere from 2x to 4x the speed of the one currently in VCB.**
* A fast preprocessor which parses circuit images almost instantly, even on circuits that take VCB many seconds to parse. 
* A more flexible assembler which remains 100% compatible with assembly programs written in VCB. (**VERY NOT SECURE.** Load trusted files only.)
* A .vcb reader which decodes the logic gates and some select settings. More to follow. (**VERY NOT SECURE.** Load trusted files only.)

# Dependencies
* ZStd
* GLM
* Gorder (included)

## License
This project is licensed under the MIT license for the time being. 
**Additionally, users agree to never distribute this library together with a graphical .vcb editor.** (Viewers are fine.)
Get the editor at https://store.steampowered.com/app/1885690/Virtual_Circuit_Board/
