
from setuptools import setup

setup(
    name='certbot-litespeed',
    packages=['certbot_litespeed'],
    install_requires=[
        'certbot',
        'zope.interface',
    ],
    entry_points={
        'certbot.plugins': [
            'litespeed = certbot_litespeed:Authenticator',
        ],
    },
)
