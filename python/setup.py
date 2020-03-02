"""
https://pypi.org/
https://pypi.org/classifiers/
"""
import pathlib
import setuptools
import os

# The directory containing this file
HERE = pathlib.Path(__file__).parent

# The text of the README file
README = (HERE / "README.md").read_text()


envs = os.environ

version = envs.get("version", "0.0.15")
# url = envs.get("url", "https://git.slock.it/in3/c/in3-python/")
url = envs.get("url", "https://github.com/slockit/in3-c")
download_url = envs.get("download_url","https://git.slock.it/in3/c/in3-python/-/archive/dev_0.1.0/in3-python-dev_0.1.0.tar.gz")
license = envs.get("license", "LGPL")
description = envs.get("description", "Incubed client and provider for web3. Based on in3-c for cython runtime.")
keywords = envs.get("keywords", "in3").split(",")

name = envs.get("name", "in3")
author = envs.get("author", "github.com/slockit")
author_email = envs.get("author_email", "products@slock.it")
# classifiers = envs.get("classifiers", "Development Status :: 3 - Alpha, Intended Audience :: Developers, Programming Language :: Python :: 3 ").split(",")




# Version is based on greater version of https://www.npmjs.com/package/in3-wasm
# Added to the minor version of this library
setuptools.setup(
    name=name,
    version=version,
    author=author,
    author_email=author_email,
    description=description,
    long_description=README,
    long_description_content_type="text/markdown",
    url=url,
    packages=setuptools.find_packages(exclude=["docs", "tests"]),
    install_requires=['web3>=4.8.0,<5.0.0', 'colorlog>=3.1.4', 'base58>=1.0.3'],
    keywords=keywords,
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: GNU Lesser General Public License v3 or later (LGPLv3+)",
        "Operating System :: OS Independent",
        "Natural Language :: English",
        "Intended Audience :: Science/Research",
        "Environment :: Console",
        "Development Status :: 2 - Pre-Alpha"
    ],
)

