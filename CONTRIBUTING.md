# How to contribute

I'm really glad you're reading this, because we need volunteer developers to help this project come to fruition.

If you haven't already, come find us in Gitter [![Gitter](https://img.shields.io/badge/Gitter-chat-blue)](https://gitter.im/slockit-in3/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge). We want you working on things you're excited about.

Here are some important resources:

 * [ReadTheDocs](https://in3.readthedocs.io/en/latest/)
 * [C API reference](https://in3.readthedocs.io/en/latest/api-c.html)
 * [C examples](https://in3.readthedocs.io/en/latest/api-c.html#examples)
 * [in3-node](https://github.com/slockit/in3-server)
 * [in3 typescript client](https://github.com/slockit/in3)
 * [Website](https://slock.it/incubed/) 
 * [Blog](https://blog.slock.it/)
 * [Incubed concept video by Christoph Jentzsch](https://www.youtube.com/watch?v=_vodQubed2A)
 * [Ethereum verification explained by Simon Jentzsch](https://www.youtube.com/watch?v=wlUlypmt6Oo)

 [![Twitter](https://img.shields.io/badge/Twitter-Page-blue)](https://twitter.com/slockitproject?s=17)
 [![Blog](https://img.shields.io/badge/Blog-Medium-blue)](https://blog.slock.it/)
 [![Youtube](https://img.shields.io/badge/Youtube-channel-blue)](https://www.youtube.com/channel/UCPOrzp3CZmdb5HJWxSjv4Ig)
 [![LinkedIn](https://img.shields.io/badge/Linkedin-page-blue)](https://www.linkedin.com/company/10327305)
 [![Gitter](https://img.shields.io/badge/Gitter-chat-blue)](https://gitter.im/slockit-in3/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge) 


## Testing

We also welcome any feedback of how you use it and what kind of challenges you may face. So when testing incubed let us know. 
Also if you want to increase our testbase, feeld free to create more test data in our testrequests.

## Submitting changes

Please send a [GitHub Pull Request to incubed](https://github.com/slockit/in3-c/pull/new/develop) with a clear list of what you've done. Please follow our coding conventions (below) and make sure all of your commits are atomic (one feature per commit).

Always write a clear log message for your commits. One-line messages are fine for small changes, but bigger changes should look like this:

    $ git commit -m "A brief summary of the commit
    > 
    > A paragraph describing what changed and its impact."

## Coding conventions

Start reading our code and you'll get the hang of it. We optimize for readability:

  * for code-formating use the [clang-format](https://github.com/slockit/in3-c/blob/master/.clang-format) provided in the repository
  * This is open source software. Consider the people who will read your code, and make it look nice for them. It's sort of like driving a car: Perhaps you love doing donuts when you're alone, but with passengers the goal is to make the ride as smooth as possible.

## Company Contribution License Agreement

For us to accept your Pull Request, we need to ask you to agree to the following Contribution License Agreement:

In order to clarify the intellectual property license granted with Contributions from any person or entity, Blockchains, LLC (**“Company”**) must have a Contributor License Agreement (**“CLA”**) on file that has been signed by each Contributor, indicating agreement to the license terms below. This license is for Your protection as a Contributor as well as the protection of Company and its users; it does not change Your rights to use Your own Contributions for any other purpose.

This CLA allows either an individual or an entity (the “Corporation”) to submit Contributions to Company, to authorize Contributions submitted by its employees or agents to Company (in the case of a Corporation), and to grant copyright and patent licenses thereto.

If You have not already done so, please complete and sign, then scan and email a PDF file of this CLA to incubed@blockchains.com.  Please read this document carefully before signing and keep a copy for your records.

Name of Individual or Corporation:__________________________________________

Mailing address:________________________________________________________

Country:_______________________________________________________________

Telephone:_____________________________________________________________

E-Mail:________________________________________________________________

You accept and agree to the following terms and conditions for Your present and future Contributions submitted to Company. Except for the license granted herein to Company and recipients of software distributed by Company, You reserve all right, title, and interest in and to Your Contributions.

1.	Definitions.
**“You”** (or **“Your”**) shall mean you as an individual (if you are signing this CLA on your own behalf) or the Corporation (if the person signing this CLA is acting on behalf of the Corporation) that is making this CLA with Company. For legal entities, the entity making a Contribution and all other entities that control, are controlled by, or are under common control with that entity are considered to be a single Contributor and this CLA shall apply to Contributions by all such entities. For the purposes of this definition, "control" means (i) the power, direct or indirect, to cause the direction or management of such entity, whether by contract or otherwise, or (ii) ownership of fifty percent (50%) or more of the outstanding shares, or (iii) beneficial ownership of such entity.
“Contribution” shall mean the code, documentation or other original work of authorship, including any modifications or additions to an existing work, that is intentionally submitted by You to Company for inclusion in, or documentation of, any of the products owned or managed by Company (the “Work”). For the purposes of this definition, “submitted” means any form of electronic, verbal, or written communication sent to Company or its representatives, including but not limited to communication on electronic mailing lists, source code control systems, and issue tracking systems that are managed by, or on behalf of, Company for the purpose of discussing and improving the Work, but excluding any communication that is conspicuously marked or otherwise designated in writing by You as “Not a Contribution.”

2.	Grant of Copyright License. Subject to the terms and conditions of this CLA, You hereby grant to Company and to recipients of software distributed by or on behalf of Company a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable copyright license to reproduce, prepare derivative works of, publicly display, publicly perform, sublicense, and distribute Your Contributions and such derivative works.

3.	Grant of Patent License. Subject to the terms and conditions of this CLA, You hereby grant to Company and to recipients of software distributed by or on behalf of Company a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable (except as stated in this section) patent license to make, have made, use, offer to sell, sell, import, and otherwise transfer the Work, where such license applies only to those patent claims licensable by You that are necessarily infringed by Your Contribution(s) alone or by combination of Your Contribution(s) with the Work to which such Contribution(s) was submitted. If any person or entity institutes patent litigation against You or any other person or entity (including a cross-claim or counterclaim in a lawsuit) alleging that Your Contribution, or the Work to which You have contributed, constitutes direct or indirect patent infringement, then any patent licenses granted to that person or entity under this CLA for that Contribution or Work shall terminate as of the date such litigation is filed.

4.	You represent that You have sufficient rights and are legally entitled to grant the above licenses with respect to each Contribution. If this CLA is being made on behalf of the Corporation, the individual signing this CLA represents that he or she is authorized to do so on behalf of the Corporation to enter into this CLA, and the Corporation represents further that each employee or agent of the Corporation designated by it is authorized to submit Contributions on behalf of the Corporation. If You are making the Contribution as an individual, and if Your present or past employer(s) or other entities for whom you have performed work has rights to intellectual property that You create that includes Your Contributions, You represent that You have received permission to make Contributions on behalf of that employer or such other entity, that Your employer or such other entity has waived such rights for Your Contributions to Company, or that Your employer or such other entity has executed a separate CLA with Company.

5.	You represent that each of Your Contributions is Your original creation (see section 8 for submissions that are not Your original creation).

6.	Based on the grant of the rights in sections 2 and 3, You acknowledge and agree that we may license Your Contributions under any license, including copyleft, permissive, commercial, or proprietary license.   

7.	You are not expected to provide support for Your Contributions, except to the extent You desire to provide support. You may provide support for free, for a fee, or not at all. Unless required by applicable law or agreed to in writing, You provide Your Contributions on an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied, including, without limitation, any warranties or conditions of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A PARTICULAR PURPOSE.

8.	Should You wish to submit work that is not Your original creation, You may submit it to Company separately from any Contribution, identifying the complete details of its source and of any license or other restriction (including, but not limited to, related patents, trademarks, and license agreements) of which You are personally aware, and conspicuously marking the work as "Submitted on behalf of a third-party: [named here]".

9.	You agree to notify Company of any facts or circumstances of which You become aware that would make these representations inaccurate in any respect.

10.	This CLA and any action related thereto will be governed by the laws of England without regard to its conflict of laws provisions. Exclusive jurisdiction and venue for actions related to this CLA will be a court of competent jurisdiction in London, England, and both parties consent to the jurisdiction of such courts with respect to any such actions. If this CLA is modified or updated by Company, Company will notify You of such updates on Company GitHub portal or will notify You via Your registered email address or via Your GitHub account (if applicable). If You do not reject the update and send Your rejection to Company within five (5) business days after Company notifies You of the change to the CLA then You will be deemed to have accepted the changed terms to the CLA. 


  
Thanks,

Your Incubed Team
