#!/usr/local/bin/python

from locust import HttpLocust, TaskSet, task
from bs4 import BeautifulSoup

class WebsiteTasks(TaskSet):
	@task
	def index(self):
		response = self.client.get("/")
		print "Locust %r" % (self.locust)
		print "Status code: %r" % (response.status_code)
		print "Content: %r" % (response.content)

		#soup = BeautifulSoup(response.content, 'html.parser')
		#hostname = soup.find(id="hostname")
		#print hostname.decode_contents(formatter="html")

class WebsiteUser(HttpLocust):
	task_set = WebsiteTasks
	min_wait = 5000
	max_wait = 10000
