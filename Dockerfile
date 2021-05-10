###############################################################################
# This file is part of the Incubed project.
# Sources: https://github.com/blockchainsllc/in3
# 
# Copyright (C) 2018-2021 slock.it GmbH, Blockchains LLC
# 
# 
# COMMERCIAL LICENSE USAGE
# 
# Licensees holding a valid commercial license may use this file in accordance 
# with the commercial license agreement provided with the Software or, alternatively, 
# in accordance with the terms contained in a written agreement between you and 
# slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
# information please contact slock.it at in3@slock.it.
# 	
# Alternatively, this file may be used under the AGPL license as follows:
#    
# AGPL LICENSE USAGE
# 
# This program is free software: you can redistribute it and/or modify it under the
# terms of the GNU Affero General Public License as published by the Free Software 
# Foundation, either version 3 of the License, or (at your option) any later version.
#  
# This program is distributed in the hope that it will be useful, but WITHOUT ANY 
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
# PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
# [Permissions of this strong copyleft license are conditioned on making available 
# complete source code of licensed works and modifications, which include larger 
# works using a licensed work, under the same license. Copyright and license notices 
# must be preserved. Contributors provide an express grant of patent rights.]
# You should have received a copy of the GNU Affero General Public License along 
# with this program. If not, see <https://www.gnu.org/licenses/>.
###############################################################################

FROM debian as build
COPY CMakeLists.txt /in3/
COPY c /in3/c/
COPY scripts /in3/scripts/
WORKDIR /in3/
USER root
RUN apt-get clean && apt-get update && apt-get install -y libcurl4-openssl-dev curl cmake build-essential
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y 
ENV PATH="/root/.cargo/bin:${PATH}"
RUN cd /in3/  && mkdir build && cd build && cmake  -DZKCRYPTO_LIB=true -DCMAKE_BUILD_TYPE=MinSizeRel -DIN3_SERVER=true  .. && make in3


FROM debian:buster-slim
COPY --from=build /in3/build/bin/in3 /bin/in3
RUN apt-get clean && apt-get update && apt-get install -y curl 
EXPOSE 8545
ENTRYPOINT ["/bin/in3"]
CMD ["--help"]
