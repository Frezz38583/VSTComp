# FET Compressor (VST3) - projet JUCE

## Prerequis (Windows)
1. Visual Studio 2022 (Community, gratuit) avec la charge "Developpement Desktop en C++"
2. CMake  (https://cmake.org/download)
3. Git    (https://git-scm.com)

## Compilation
Dans ce dossier, ouvre un terminal :

    cmake -B build -G "Visual Studio 17 2022"
    cmake --build build --config Release

(le premier build telecharge JUCE et prend quelques minutes)

Le plugin est ici :
    build\FETCompressor_artefacts\Release\VST3\FET Compressor.vst3

Copie le dossier .vst3 dans  C:\Program Files\Common Files\VST3

## FL Studio
Options > Manage plugins > Find installed plugins (Start scan).
Le plugin apparait dans la liste, ajoute-le sur une piste du mixer.
