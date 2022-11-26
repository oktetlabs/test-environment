Graphical representation of a measurement result
================================================

The overall measurement results processing logic:

```text
+-------------------+                              +------------------+
| +---------------+ |                           +->|  HTML Generator  |
| |Test:          | |                           |  +------------------+
| |  - API calls  | |            +-----------+  |
| |               | |            |Json       |  |
| +---------------+ |  MI LOG    |           |  |  +------------------+
| +---------------+ |----------->|           |--|->|     Text log     |
| |Run:           | |            |           |  |  +------------------+
| |  - data       | |            +-----------+  |
| |    collection | |                           |  +------------------+
| +---------------+ |                           +->|  HTML Frontends  |
+-------------------+                              +------------------+
```

first section will describe the JSON logic and how it should be processed by
various backends. **Should** does not mean it **MUST** be processed this way.

We describe a **view** that is one of the ways to look at the data in the
measurement result. It records semantics that is known to the test author and
outlines it for the tools that might use it.

It's obvious that one won't draw a 3d diagram in a text log. Or that if
you're using an HTML frontend that user can interact with you can describe
other ways to represent the data.

Representation
--------------

For the time being we support:

  * `line-graph` representation - it's a 2d graph that shows you `Y(x)`
    function.

### JSON ###

MI of type `measurement` consists of:

  * info on how it was collected + how to match with measurements from other
    results(format, tool, keys),
  * **data** : lives in `results` subtree,
  * **views** : ways to look at the data if you're a machine.

NOTE: to some extend it's breaking the data/separation rule. Said that
we're separating them as much as possible while keeping them within one
measurement. Reason is that creating a separate object and association
between those objects to mark that this 'view' is applicable to this specific
'measurement' was decided to be an unnecessary complication.

#### Views ####

Views represent how one **might** view the data. When we're using interactive
interfaces (like bublik) one can tweak or completely override it. When we're
talking about fully automatic logs generation those are the instructions for
the RGT tool.

Assuming we have a very generic artifact that has 4 measurements:

```
⊖{
    "type": "measurement",
    "version": 1,
    "tool": "mytool",
    "results": ⊖[
       ⊖{
            "type": "temperature",
            "name": "temp",
            "entries": ⊖[
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...}
            ]
        },
       ⊖{
            "type": "pps",
            "name": "foops",
            "entries": ⊖[
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...}
            ]
        },
       ⊖{
            "type": "latency",
            "name": "app_latency",
            "entries": ⊖[
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...}
            ]
        },
       ⊖{
            "type": "time",
            "name": "timestamp",
            "entries": ⊖[
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...},
               ⊕{...}
            ]
        },
    ],
}
```

**NOTE**: it's also assumed that if we want `Y(x)` function then arrays for
`x` and `Y` have the same length. If it's not the case the MI interface user
should generate a clear warnings displayed next to the graph.

#### Default ####

Nothing will be drawn. System makes no guesses.

#### 1 diagram with 3 graphics with 'timestamp' as base axis ####

```json
views {
    [
       {
           "name": "graph",
           "type": "line-graph",
           "title": "How other measurements depend on timestamp",
           "axis-x": { "type": "time", "name": "timestamp" }
       }
    ]
}
```


#### Sequential graphs ####

User likes to say that `app_latency` is measured every N packets, so we can
simply assume that there is equal interval between the measurements:

```json
views {
    [
        {
            "name": "graph",
            "type": "line-graph",
            "title": "How app_latency and foops change with sequence number",
            "axis-x": { "name": "auto-seqno" },
            "axis-y": [ { "type": "latency", "name": "app_latency" },
                        { "type": "pps", "name": "foops" } ],
        }
    ]
}
```

will draw two lines on the same graph:

 * `app_latency(measurement_number)`
 * `foops(measurement_number)`.

which is why it's called 'auto'.

#### Full line specification ####

Fully specified graph of `foops(timestamp)`:

```json
views {
    [
       {
           "name": "graph",
           "type": "line-graph",
           "title": "How foops depends on timestamp",
           "axis-x": { "type": "time", "name": "timestamp" },
           "axis-y": [ { "type": "pps", "name": "foops" } ]
       }
    ]
}
```

Two **separate** line-graphs:

```json
views {
    [
       {
           "name": "graph1",
           "type": "line-graph",
           "title": "How app_latency depends on timestamp",
           "axis-x": { "type": "time", "name": "timestamp" },
           "axis-y": [ { "type": "latency", "name": "app_latency" } ]
       },
       {
           "name": "graph2",
           "type": "line-graph",
           "title": "How foops depends on timestamp",
           "axis-x": { "type": "time", "name": "timestamp" },
           "axis-y": [ { "type": "pps", "name": "foops" } ]
       }
    ]
}
```

By design you cannot combine `foops(timestamp)` and `app_latency(foops)` on
the same graph.
