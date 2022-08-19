#!/usr/bin/env perl

use strict;
use warnings;

use Mojolicious::Lite;
use Mojo::JSON qw(true false);
use JSON qw(to_json);

my @clients;

my $current_plan;
my $stop = false;

open my $log, ">", "weblog";

post '/init' => sub {
    my ($c) = @_;

    my $json = $c->req->json;
    app->log->debug("got an init with the following message:\n", $c->req->body);

    print $log "INIT\n";
    print $log to_json($json, {utf8 => 1, pretty => 1});
    print $log "\n";

    $current_plan = to_json($json->{plan});
    $stop = false;

    $_->write("event: plan\ndata: $current_plan\n\n") for @clients;
    $c->render(json => {runid => 0});
};

post '/feed' => sub {
    my ($c) = @_;
    my $run = $c->param('run');

    my $data = $c->req->body;
    app->log->debug("got some feed for run $run:\n", $data);

    print $log "FEED\n";
    print $log to_json($c->req->json, {utf8 => 1, pretty => 1});
    print $log "\n";

    $_->write("event: feed\ndata: $data\n\n") for @clients;
    $c->render(json => { stop => $stop});
};

post '/finish' => sub {
    my ($c) = @_;
    my $run = $c->param('run');

    app->log->debug("run $run finished");

    print $log "FINISH\n";
    print $log to_json($c->req->json, {utf8 => 1, pretty => 1});
    print $log "\n";

    $_->write("event: finish\n\n") for @clients;
    $c->render(text => '', status => 204);
};

post '/stop' => sub {
    $stop = true;
};

get '/' => sub {
    my ($c) = @_;
    $c->redirect_to('/index');
};

get '/index' => sub {
    my ($c) = @_;
    $c->render(template => 'index');
};

get '/events' => sub {
    my ($c) = @_;
    $c->res->headers->content_type('text/event-stream');
    $c->write;
    $c->inactivity_timeout(300);
    push @clients, $c;
    $c->on(finish => sub {
        my $ind = 0;
        $ind++ until $clients[$ind] == $c;
        splice @clients, $ind, 1;
    });
    if ($current_plan) {
        $c->write("event: plan\ndata: $current_plan\n\n");
    }
};


app->start;

__DATA__

@@ index.html.ep

<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <link rel="stylesheet" href="style.css">
    <title>TE Live Results</title>
</head>
<body>

<button id="stop_button">Stop execution</button>

<div id="root">
<span class="caret caret-down">ts</span>
<ul>
    <li><div>tcp</div></li>
    <li><div>udp</div></li>
    <li><div><span class="caret caret-down">tcp udp</span><ul><li><span>child</span></li></ul></div></li>
</ul>

</div>

<script>
    var toggler = document.getElementsByClassName('caret');
    var i;
    var es = new EventSource('/events');
    var children = [];
    var node2plan = [-1];

    var stop_button = document.getElementById('stop_button');
    stop_button.addEventListener('click', function(event) {
        event.preventDefault();
        let xhr = new XMLHttpRequest();
        xhr.open("POST", "/stop", true);
        xhr.send();
    });

    function normalize_type(type)
    {
        switch (type) {
            case 'PACKAGE':
            case 'pkg':
                return 'pkg';
            case 'SESSION':
            case 'session':
                return 'session';
            case 'TEST':
            case 'test':
                return 'test';
        }
    }

    function process_plan(plan, parent, role)
    {
        let children_list;

        plan.type = normalize_type(plan.type);
        let name = document.createElement('span');
        name.textContent = (role || '') + ' ' + plan.type + ' ' + plan.name;
        parent.appendChild(name);

        let status = document.createElement('span');
        parent.appendChild(status);

        children.push({
            type: plan.type,
            name: plan.name,
            status: status,
        });

        if (plan.prologue || plan.keepalive || plan.epilogue || plan.children)
        {
            children_list = document.createElement('ul');
            parent.appendChild(children_list);

            name.classList.add('caret', 'caret-down');
            name.addEventListener("click", function() {
                this.parentElement.querySelector("ul").classList.toggle("hidden");
                this.classList.toggle("caret-down");
            });

        }

        if (plan.prologue)
        {
            let li = document.createElement('li');
            children_list.appendChild(li);
            process_plan(plan.prologue, li, 'prologue')
        }

        if (plan.children && plan.children.length > 0)
        {
            plan.children.forEach((item) => {
                for (let i = 0; i < (item.iterations || 1); i++)
                {
                    if (plan.keepalive)
                    {
                        let li = document.createElement('li');
                        children_list.appendChild(li);
                        process_plan(plan.keepalive, li, 'keepalive');
                    }
                    if (item.type != 'skipped')
                    {
                        let li = document.createElement('li');
                        children_list.appendChild(li);
                        process_plan(item, li);
                    }
                }
            });
        }

        if (plan.epilogue)
        {
            let li = document.createElement('li');
            children_list.appendChild(li);
            process_plan(plan.epilogue, li, 'epilogue');
        }
    }

    es.addEventListener('plan', function(e) {
        let root = document.getElementById('root');
        let msg = JSON.parse(e.data);

        root.innerHTML = '';
        children = [];
        process_plan(msg, root);
    });

    es.addEventListener('feed', function(e) {
        let arr = JSON.parse(e.data);

        arr.forEach((item) => {
            let pid = item.plan_id;
            console.log(item);


            if (item.type == 'test_end')
            {
                if (pid == null)
                {
                    console.log('test_end pid is null');
                    return;
                }
                children[pid].status.textContent = item.obtained.status;
            }
            else if (item.type == 'test_start')
            {
                if (pid == null)
                {
                    console.log('test_start pid is null');
                    return;
                }

                node2plan[item.id] = pid;

                if (children[pid].type != item.node_type ||
                    children[pid].name != item.name)
                {
                    console.log(`plan (id ${pid}) mismatch!\n` +
                                `expected ${children[pid].type + ' ' + children[pid].name}\n` +
                                `got ${item.node_type + ' ' + item.name}`);
                }
            }
            else if (item.type == 'artifact')
            {
                let plan_id = node2plan[item.test_id];
                let test = children[plan_id].status.parentElement;
                let ul = test.getElementsByTagName('ul');
                let list = null;

                console.log(test);

                if (ul.length > 0)
                {
                    list = ul.item(0);
                }
                else
                {
                    list = document.createElement('ul');
                    test.appendChild(list);

                    test.classList.add('caret');
                    list.classList.add("hidden");
                    test.addEventListener("click", function() {
                        test.querySelector("ul").classList.toggle("hidden");
                        test.classList.toggle("caret-down");
                    });
                }

                let listitem = document.createElement('li');

                let title = document.createElement('span');
                title.textContent = 'artifact';
                listitem.appendChild(title);


                let body = document.createElement('div');
                body.textContent = item.body;
                listitem.appendChild(body);

                list.appendChild(listitem);

            }
        });
    });

    for (i = 0; i < toggler.length; i++)
    {
        toggler[i].addEventListener("click", function() {
            this.parentElement.querySelector("ul").classList.toggle("hidden");
            this.classList.toggle("caret-down");
        });
    }
</script>
</body>
</html>

@@ style.css

.hidden {
    display: none;
}

.caret {
    cursor: pointer;
}

.caret::before {
    content: "\25B6";
    color: black;
    display: inline-block;
    margin-right: 0.2em;
}

.caret-down::before {
    transform: rotate(90deg);
}

ul {
    margin: 0;
    display: block;
}
