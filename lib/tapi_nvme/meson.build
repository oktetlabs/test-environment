# Copyright (C) 2018 OKTET Labs. All rights reserved.

headers += files(
    'tapi_nvme.h',
    'tapi_nvme_kern_target.h',
    'tapi_nvme_onvme_target.h',
)
sources += files(
    'tapi_nvme.c',
    'tapi_nvme_kern_target.c',
    'tapi_nvme_onvme_target.c',
    'tapi_nvme_internal.c',
)
deps += [
    dep_lib_rpcxdr,
    dep_lib_tapi_rpc,
    dep_lib_tapi_job,
]
