import pathlib
import setuptools
from os import environ

"""
Information on how to build this file
https://pypi.org/
https://pypi.org/classifiers/
"""


version = "2.3.2rc10"
url = environ.get("url", "https://github.com/slockit/in3-c")
License = environ.get("license", "AGPL")
description = environ.get(
    "description", "Incubed client and provider for web3. Based on in3-c runtime.")
keywords = environ.get(
    "keywords", "in3,c,arm,x86,x64,macos,windows,linux,blockchain,ethereum,bitcoin,ipfs").split(",")
readme = (pathlib.Path(__file__).parent / "README.md").read_text()
name = environ.get("name", "in3")
author = environ.get("author", "github.com/slockit/in3-c")
author_email = environ.get("author_email", "products@slock.it")
setuptools.setup(
    name=name,
    include_package_data=True,
    version=version,
    author=author,
    author_email=author_email,
    description=description,
    long_description=readme,
    long_description_content_type="text/markdown",
    url=url,
    packages=setuptools.find_packages(exclude=["docs", "tests"]),
    install_requires=['requests==2.23.0'],
    keywords=keywords,
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: GNU Affero General Public License v3 or later (AGPLv3+)",
        "Operating System :: OS Independent",
        "Natural Language :: English",
        "Intended Audience :: Science/Research",
        "Environment :: Console",
        "Development Status :: 4 - Beta"
    ]
)
