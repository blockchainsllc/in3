import pathlib
import setuptools
import os

"""
Information on how to build this file
https://pypi.org/
https://pypi.org/classifiers/
"""

envs = os.environ
version = envs.get("version", "0.0.15")
url = envs.get("url", "https://github.com/slockit/in3-c")
download_url = envs.get("download_url",
                        "https://git.slock.it/in3/c/in3-python/-/archive/dev_0.1.0/in3-python-dev_0.1.0.tar.gz")
License = envs.get("license", "LGPL")
description = envs.get("description", "Incubed client and provider for web3. Based on in3-c for cython runtime.")
keywords = envs.get("keywords", "in3").split(",")
readme = (pathlib.Path(__file__).parent / "README.md").read_text()
name = envs.get("name", "in3")
author = envs.get("author", "github.com/slockit")
author_email = envs.get("author_email", "products@slock.it")
setuptools.setup(
    name=name,
    version=version,
    author=author,
    author_email=author_email,
    description=description,
    long_description=readme,
    long_description_content_type="text/markdown",
    url=url,
    packages=setuptools.find_packages(exclude=["docs", "tests"]),
    install_requires=['requests=>2.23.0'],
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
