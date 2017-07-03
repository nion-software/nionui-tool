set -e # fail script if any command fails

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  xcodebuild -project NionUILauncher.xcodeproj -target "Nion UI Launcher" -configuration Release
  cd build/Release
  zip -r NionUILauncher-Mac.zip Nion\ UI\ Launcher.app
  mkdir -p ../../release
  mv NionUILauncher-Mac.zip ../../release
  cd ../..
fi

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu trusty universe"
  sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu trusty main"
  sudo apt-get update
  sudo apt-get install qmlscene qt5-default qt5-qmake libqt5svg5-dev -y
  sudo unlink /usr/bin/g++ && sudo ln -s /usr/bin/g++-5 /usr/bin/g++
  gcc --version
  wget https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh -O miniconda.sh
  bash miniconda.sh -b -p $HOME/miniconda
  export PATH="$HOME/miniconda/bin:$PATH"
  hash -r
  conda install --yes numpy=1.12
  conda info -a
  bash linux_build.sh ~/miniconda
  mkdir release
  cd linux/x64
  zip NionUILauncher-Linux.zip *
  mkdir -p ../../release
  cp NionUILauncher-Linux.zip ../../release
  cd ../..
fi
