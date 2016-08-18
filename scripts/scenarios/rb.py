import sys
import json
import requests
from collections import namedtuple

from scenarios.rb_api_token import take_api_token

# link: http://stackoverflow.com/questions/6578986#15882054
def _json_object_hook(d):
    return namedtuple('X', d.keys())(*d.values())

class ReviewBoard(object):
    def __init__(self, server_api=None, api_token=None):
        self.server_api = server_api
        if not self.server_api:
            self.server_api = 'https://reviewboard.oktetlabs.ru/api'
        self.api_token = api_token
        if not self.api_token:
            self.api_token = take_api_token()
        assert self.api_token

    def _get(self, href):
        r = requests.get(href, headers = {
                "Authorization": 'token ' + self.api_token,
            })
        data = r.content.decode('utf-8')
        content = json.loads(data, object_hook=_json_object_hook)
        assert content.stat == 'ok', data
        return content

    def get_repositories(self):
        return self._get('%s/repositories/' % (self.server_api))

    def _validate_repository(self, repository):
        items = []
        repositories = self.get_repositories().repositories
        for r in repositories:
            if repository == r.id or repository == r.name:
                return r.id
            if repository in r.name:
                items.append(r)
        if len(items) == 1:
            return items[0].id
        if items:
            repositories = items
        print('Unknown repository, list available repository:')
        for r in repositories:
            print('  %i) %s' % (r.id, r.name))
        sys.exit(-1)

    def create_review(self, repository):
        repository = self._validate_repository(repository)
        href = '%s/review-requests/' % (self.server_api)
        r = requests.post(href, headers = {
                "Authorization": "token " + self.api_token,
            }, data={
                "repository":    repository,
            })
        data = r.content.decode('utf-8')
        content = json.loads(data, object_hook=_json_object_hook)
        assert content.stat == 'ok', data
        return content

    def update_diff(self, review_id, data=None, filename=None):
        if not data:
            data = open(filename, 'r').read()
        href = '%s/review-requests/%s/diffs/' % (self.server_api, review_id)
        r = requests.post(href, headers = {
                "Authorization": "token " + self.api_token
            }, files = {
                'path': data
            })
        data = r.content.decode('utf-8')
        content = json.loads(data, object_hook=_json_object_hook)
        assert content.stat == 'ok', data
        return content

    def review(self, review_id):
        href = '%s/review-requests/%s/' % (self.server_api, review_id)
        r = requests.get(href, headers = {
                "Authorization": "token " + self.api_token
            })
        data = r.content.decode('utf-8')
        content = json.loads(data, object_hook=_json_object_hook)
        assert content.stat == 'ok', data
        return content

    def change_description(self, review_id, description, text_type='markdown'):
        href = '%s/review-requests/%s/draft/' % (self.server_api, review_id)
        r = requests.post(href, headers = {
                "Authorization": "token " + self.api_token,
            }, data={
                "changedescription_text_type": text_type,
                "changedescription": description,
            })
        data = r.content.decode('utf-8')
        content = json.loads(data, object_hook=_json_object_hook)
        assert content.stat == 'ok', data
        return content
