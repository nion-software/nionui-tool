set -e # fail script if any command fails

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  rm -rf dist
  rm -rf launcher/build/Release
  echo "Building..."
  xcodebuild -project launcher/NionUILauncher.xcodeproj -target "Nion UI Launcher" -configuration Release
  rm -rf launcher/build/Release/*.dSYM
  pushd launcher/build/Release
  echo "Zipping..."
  tar zcf LauncherApp.tar.gz Nion\ UI\ Launcher.app
  pwd
  ls -l
  popd
  python3 --version
  python3 setup.py bdist_wheel
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
  conda install --yes numpy=1.14 packaging
  conda info -a
  pushd launcher
  rm -rf linux/x64
  echo "Building..."
  find $HOME/miniconda -name "Python.h"
  find $HOME/miniconda -name "arrayobject.h"
  bash linux_build.sh $HOME/miniconda
  echo "Zipping..."
  tar zcf launcher_app.tar.gz linux/x64
  pwd
  ls -l linux/x64
  popd
  $HOME/miniconda/bin/python --version
  $HOME/miniconda/bin/python setup.py bdist_wheel
fi
