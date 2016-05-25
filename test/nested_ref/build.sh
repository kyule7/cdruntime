#!/bin/bash
mpicxx -std=gnu++0x -g -o nested_kref nested_kref.C -I../../include -L../../lib -lcd
