Mesh Module
  |--api	Mesh APIs of Stack and Service
  |     |--*.h			Header Files of APIs
  |--lib	Mesh Stack and Model Libraries
  |     |--ble6_mesh.lib		Mesh Support general node (no Relay Friend LPN)
  |     |--ble6_mesh_full.lib	Mesh Support all node feature: Relay Friend LPN Proxy
  |     |--sig_model.lib		SIG Mesh Model, Genrated via "\mdk\model.uvprojx"
  |     |--sig_model.h		wrap *.h file which be in "\model\api"
  |--mdk	MDK5 Project of Mesh Model
  |     |--model.uvprojx	Build sig_model.lib
  |     |--model.uvoptx		
  |--model	Mesh Model Source File
  |     |--api		APIs of Mesh Model 
  |     |--gen		Mesh Generic Model
  |     |--light		Mesh Light Model
  |     |--sens		Mesh Sensor Model*
  |     |--tscn		Mesh Time and Scene Model*
  |--genie	Tmall Genie Source File
  |     |--genie_mesh.c		Tmall Genie mesh
  |     |--genie_mesh.h		
  |     |--sha256.c		TinyCrypt SHA-256
  |     |--sha256.c		

Mesh Demo
  see ..\projects\bleMesh