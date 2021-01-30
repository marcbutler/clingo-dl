'''
Script to build pip source package for clingo-dl.
'''

from re import finditer, escape, match, sub
from subprocess import check_call, check_output


def adjust_version():
    '''
    Adjust version in setup.py.
    '''
    pip = check_output(['curl', '-sL', 'https://test.pypi.org/simple/clingo-dl']).decode()
    version = None
    with open('libclingo-dl/clingo-dl.h') as fh:
        for line in fh:
            m = match(r'#define CLINGODL_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"', line)
            if m is not None:
                version = m.group(1)

    assert version is not None

    post = 0
    for m in finditer(r'clingo[-_]dl-{}\.tar\.gz'.format(escape(version)), pip):
        post = max(post, 1)

    for m in finditer(r'clingo[-_]dl-{}\.post([0-9]+)\.tar\.gz'.format(escape(version)), pip):
        post = max(post, int(m.group(1)) + 1)

    for m in finditer(r'clingo[-_]dl-{}\.post([0-9]+).*\.whl'.format(escape(version)), pip):
        post = max(post, int(m.group(1)))
    print(post)

    with open('setup.py') as fr:
        setup = fr.read()
    with open('setup.py', 'w') as fw:
        if post > 0:
            fw.write(sub('version( *)=.*', 'version = \'{}.post{}\','.format(version, post), setup, 1))
        else:
            fw.write(sub('version( *)=.*', 'version = \'{}\','.format(version), setup, 1))


if __name__ == "__main__":
    adjust_version()
    #check_call(['python3', 'setup.py', 'sdist'])
