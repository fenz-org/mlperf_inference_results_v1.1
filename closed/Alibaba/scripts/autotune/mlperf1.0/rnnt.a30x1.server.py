# "Typical" format which doesn't differ much from JSON

# gpu_batch_size = [ 1792, 2048 ]
gpu_batch_size = [ 2048 ]
audio_batch_size = [ 2048, 4096 ]
audio_buffer_num_lines = [ 1024, 2048 ]
server_target_qps = [ 40000, 41000, 42000 ]

# ... But, we can do arbitrary computation to calculate other variables
# Variables with leading underscores are not considered "public", and should be used for private computation

# We can even use external libraries to do wild stuff:
# Note that functions which are not preceeded by META_ are considered "private" and not exposed to grid.py/the scheduler.

# We have some a posteriori knowledge that certain parameters are only meaningful at runtime and not build time
# This meta function let's the scheduler know that to order runs in such a way to minimize the number of rebuilds
def META_get_no_rebuild_params():
    return ["server_target_qps"]

# It's sometimes easier to just declare some arbitrary list of parameters, and then describe a predicate which filters that list.
# Note well that we never specify "audio_buffer_num_lines" because config is the updated default config.
def META_is_config_valid(config):
    return config["audio_batch_size"] <= config["audio_buffer_num_lines"]
