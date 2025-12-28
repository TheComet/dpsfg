import sys
import os

this_dir = os.path.dirname(os.path.abspath(__file__))
if this_dir not in sys.path:
    sys.path.insert(0, this_dir)

from rb import rb_PrettyPrinter
from vec import vec_PrettyPrinter
from bmap import bmap_PrettyPrinter
from hmap import hmap_PrettyPrinter
from mat import mat_PrettyPrinter
from poly import cpoly_PrettyPrinter, rpoly_PrettyPrinter, \
    pfd_poly_PrettyPrinter
from s import s_PrettyPrinter
from strspan import strspan_PrettyPrinter
from strview import strview_PrettyPrinter
from strlist import strlist_PrettyPrinter

def factories(val):
    if str(val.type).endswith("_rb *"):
        return rb_PrettyPrinter(val)
    if str(val.type).endswith("_vec *"):
        return vec_PrettyPrinter(val)
    if str(val.type).endswith("csfg_cpoly *"):
        return cpoly_PrettyPrinter(val)
    if str(val.type).endswith("csfg_rpoly *"):
        return rpoly_PrettyPrinter(val)
    if str(val.type).endswith("csfg_pfd_poly *"):
        return pfd_poly_PrettyPrinter(val)
    if str(val.type).endswith("csfg_mat *"):
        return mat_PrettyPrinter(val)
    if str(val.type).endswith("csfg_mat_reorder *"):
        return vec_PrettyPrinter(val)
    if str(val.type).endswith("_bmap *"):
        return bmap_PrettyPrinter(val)
    if str(val.type).endswith("_hmap *"):
        return hmap_PrettyPrinter(val)
    if str(val.type).endswith("str *"):
        return s_PrettyPrinter(val)
    if str(val.type).endswith("strspan"):
        return strspan_PrettyPrinter(val)
    if str(val.type).endswith("strview"):
        return strview_PrettyPrinter(val)
    if str(val.type).endswith("strlist *"):
        return strlist_PrettyPrinter(val)


gdb.pretty_printers.append(factories)

import plot
