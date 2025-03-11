# vox2bella
Prototype learning program. Command line converter from magicaVoxel .vox format to DiffuseLogic's .bsz 

## Usage
```
vox2bella -vi:chr_knight.vox
```
![chr_knight](resources/chr_knight.jpg)

## Build

## Dependencies
Download latest [bella_scene_sdk](https://bellarender.com/builds/) double click and drag embedded directory into same parent as this repo.


### Linux and Mac
```
cd vox2bella
make
```

### Windows
```
cd vox2bella
msbuild vox2bella.vcxproj /p:Configuration=release /p:Platform=x64 /p:PlatformToolset=v143
```



