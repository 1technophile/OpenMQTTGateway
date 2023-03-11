# Development contributions

We like pull requests from everyone. By participating in this project, you
agree to follow the code of conduct below

[code of conduct](https://github.com/1technophile/OpenMQTTGateway/blob/master/CODE_OF_CONDUCT.md)

[code style guide](https://google.github.io/styleguide/cppguide.html#Formatting)

So as to format automatically your document you have to add the "clang-Format" extension to VSCode, once done, you can format the code by doing a right click into the code file window and clicking "Format document".

Fork the [development branch](https://github.com/1technophile/OpenMQTTGateway/tree/development), then clone the repo

Make your modification,
* If you want to add a new gateway, name it `ZgatewayXXX`, `XXX` replaced by your gateway communication type, can be more than three letters
* If you want to add a new sensor, name it `ZsensorYYY`, `YYY` replaced by your sensor type, can be more than three letters
* If you want to add a new actuator, name it `ZactuatorZZZ`, `ZZZ` replaced by your actuator type, can be more than three letters

Review your code, compile it for Arduino Uno, ESP32 and ESP8266

Test it locally on your hardware config

Emit a pull request

Verify the GitHub Actions CI compilation results

Request for review

We may suggest some changes, improvements or alternatives.

Some things that will increase the chance that your pull request is accepted:
* Comment your code,
* Ask eventually for design guidelines,
* Write a [good commit message][commit].

[commit]: http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html

## Nightly builds

Nightly builds can be found [here](https://docs.openmqttgateway.com/dev/upload/web-install.html) for testing purposes only.
They are generated from the development branch nightly or from a particular pull request upon request.

## Developer Certificate Of Origin

```
    By making a contribution to this project, I certify that:

    (a) The contribution was created in whole or in part by me and I
        have the right to submit it under the open source license
        indicated in the file; or

    (b) The contribution is based upon previous work that, to the best
        of my knowledge, is covered under an appropriate open source
        license and I have the right under that license to submit that
        work with modifications, whether created in whole or in part
        by me, under the same open source license (unless I am
        permitted to submit under a different license), as indicated
        in the file; or

    (c) The contribution was provided directly to me by some other
        person who certified (a), (b) or (c) and I have not modified
        it.

    (d) I understand and agree that this project and the contribution
        are public and that a record of the contribution (including all
        personal information I submit with it, including my sign-off) is
        maintained indefinitely and may be redistributed consistent with
        this project or the open source license(s) involved.
```

This Developer Certificate Of Origin (DCO) was adopted on June 7, 2021.

The text of this license is available under the [Creative Commons Attribution-ShareAlike 3.0 Unported License](http://creativecommons.org/licenses/by-sa/3.0/).  It is based on the Linux [Developer Certificate Of Origin](http://elinux.org/Developer_Certificate_Of_Origin).

To accept the DCO it is required to put a x between [ ] on `[ ] I accept the DCO` in the PR template when submitting it. The [ ] is an opt-in box, so you have to manually accept it.
