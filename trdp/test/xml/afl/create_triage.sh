#!/bin/bash

cwtriage -root afl-out/crashes/ -match id -- ./trdp-xmlprint-test @@ > classification_cw_triage.txt

