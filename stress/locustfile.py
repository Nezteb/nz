#!/usr/local/bin/python

from locust import HttpLocust, TaskSet, task
from bs4 import BeautifulSoup

logfile = open("./output.log", "a")

class WebsiteTasks(TaskSet):
	@task
	def index(self):
		response = self.client.get("")
		logfile.write("Locust %r: " % (self.locust))
		soup = BeautifulSoup(response.content, 'html.parser')
		hostname = soup.find(id="hostname")
		logfile.write(hostname.decode_contents(formatter="html"))
		logfile.write("\n")

class WebsiteUser(HttpLocust):
	task_set = WebsiteTasks
	min_wait = 1000
	max_wait = 5000
