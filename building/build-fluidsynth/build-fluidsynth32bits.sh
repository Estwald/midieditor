#!/usr/bin/env bash

# QT path (i.e c:/QT)
MYPATH=$1 
SHARED=$2
CMD=$3

FSVERSION="2.4.7"

if [[ "$CMD" == "" ]]; then
	echo Error! CMD empty
exit 1
fi

if [[ "$MYPATH" == "" ]]; then MYPATH=F:/QT
fi
if [[ "$SHARED" == "" ]]; then SHARED="OFF"
fi

if [[ "$SHARED" == "OFF" ]]; then PSHARED="-static"
	echo Building Fluidsynth static library
else
	PSHARED=""
        echo Building Fluidsynth shared library
fi

set PATH=${MYPATH}/msys32/usr/bin;

#export PKG_CONFIG_PATH="/mingw32/lib/pkgconfig-static:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="/mingw32/lib/pkgconfig-static:/mingw32/lib/pkgconfig"

export CXXFLAGS="-static-libgcc -static-libstdc++"
export LDFLAGS="-static -static-libgcc -static-libstdc++"

# version estatica


if [[ "$CMD" == "config" ]]; then

	if [ ! -d ${MYPATH}/fluidsynth-${FSVERSION} ]; then
		git clone --branch v${FSVERSION} --depth 1 https://github.com/FluidSynth/fluidsynth.git "${MYPATH}/fluidsynth-${FSVERSION}"

	fi

	# patch with iconv library

	#FILE1="${MYPATH}/fluidsynth-${FSVERSION}/src/CMakeLists.txt"
	#chmod u+rw "${FILE1}"	

	#if ! grep -q '#hack iconv' "${FILE1}"; then
    	#	sed -i '/\${LIBFLUID_LIBS}/a\    iconv #hack iconv' "${FILE1}"
    	#echo "src/CMakeLists.txt patched"
	#fi


	cd  "${MYPATH}"
	rm -rf fluidsynth-build-32${PSHARED}
	mkdir fluidsynth-build-32${PSHARED} && cd fluidsynth-build-32${PSHARED}

	cmake -G "Ninja"  ${MYPATH}/fluidsynth-${FSVERSION} \
  		-DCMAKE_BUILD_TYPE=Release \
  		-DCMAKE_IGNORE_PATH="/mingw32" \
  		-DCMAKE_INSTALL_PREFIX="${MYPATH}/fluidsynth-32${PSHARED}" \
  		-DCMAKE_C_COMPILER="${MYPATH}/msys64/mingw32/bin/gcc.exe" \
  		-DCMAKE_CXX_COMPILER="${MYPATH}/msys64/mingw32/bin/g++.exe" \
  		-DBUILD_SHARED_LIBS=${SHARED} \
  		-Denable-qt=ON \
  		-Denable-shared=OFF \
  		-Denable-static=ON \
  		-Denable-floats=ON \
  		-DCMAKE_C_FLAGS_RELEASE="-O2 -msse4.2 -mavx" \
  		-DCMAKE_CXX_FLAGS_RELEASE="-O2 -msse4.2 -mavx" \
  		-Denable-openmp=OFF \
  		-Denable-dbus=OFF \
  		-Denable-ladspa=OFF \
  		-Denable-alsa=OFF \
  		-Denable-jack=OFF \
  		-Denable-readline=OFF -Denable-dsound=OFF -Denable-wasapi=OFF \
  		-DCMAKE_C_FLAGS="-DGLIB_STATIC_COMPILATION -DGOBJECT_STATIC_COMPILATION -DGMODULE_STATIC_COMPILATION"
		sync

elif [[ "$CMD" == "build" ]]; then

	echo Build
	cd ${MYPATH}/fluidsynth-build-32${PSHARED}
	ninja
	sync

elif [[ "$CMD" == "install" ]]; then

        echo Install
	cd ${MYPATH}/fluidsynth-build-32${PSHARED}
	ninja install
	sync

elif [[ "$CMD" == "clean" ]]; then

	echo Clean
	rm -rf ${MYPATH}/fluidsynth-build-32${PSHARED}
	sync
fi
